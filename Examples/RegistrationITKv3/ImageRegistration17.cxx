/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

// Software Guide : BeginLatex
//
//  This example illustrates how to do registration with a 2D Translation Transform,
//  the Mutual Information Histogram metric and the Amoeba optimizer.
//
// Software Guide : EndLatex


// Software Guide : BeginCodeSnippet
#include "itkImageRegistrationMethod.h"

#include "itkTranslationTransform.h"
#include "itkMutualInformationHistogramImageToImageMetric.h"
#include "itkAmoebaOptimizer.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"


//  The following section of code implements a Command observer
//  used to monitor the evolution of the registration process.
//
#include "itkCommand.h"
class CommandIterationUpdate : public itk::Command
{
public:
  using Self = CommandIterationUpdate;
  using Superclass = itk::Command;
  using Pointer = itk::SmartPointer<Self>;
  itkNewMacro( Self );

protected:
  CommandIterationUpdate()
    {
    m_IterationNumber=0;
    }

public:
  using OptimizerType = itk::AmoebaOptimizer;
  using OptimizerPointer = const OptimizerType   *;

  void Execute(itk::Object *caller, const itk::EventObject & event) override
    {
    Execute( (const itk::Object *)caller, event);
    }

  void Execute(const itk::Object * object, const itk::EventObject & event) override
    {
    OptimizerPointer optimizer = static_cast< OptimizerPointer >( object );
    if( ! itk::IterationEvent().CheckEvent( &event ) )
      {
      return;
      }
    std::cout << m_IterationNumber++ << "   ";
    std::cout << optimizer->GetCachedValue() << "   ";
    std::cout << optimizer->GetCachedCurrentPosition() << std::endl;
    }

private:
  unsigned long m_IterationNumber;
};

int main( int argc, char *argv[] )
{
  if( argc < 4 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " fixedImageFile  movingImageFile ";
    std::cerr << " outputImagefile ";
    std::cerr << " [initialTx] [initialTy]" << std::endl;
    return EXIT_FAILURE;
    }

  constexpr unsigned int Dimension = 2;
  using PixelType = unsigned char;

  using FixedImageType = itk::Image< PixelType, Dimension >;
  using MovingImageType = itk::Image< PixelType, Dimension >;

  using TransformType = itk::TranslationTransform< double, Dimension >;

  using OptimizerType = itk::AmoebaOptimizer;
  using InterpolatorType = itk::LinearInterpolateImageFunction<
                                    MovingImageType,
                                    double             >;
  using RegistrationType = itk::ImageRegistrationMethod<
                                    FixedImageType,
                                    MovingImageType    >;


  using MetricType = itk::MutualInformationHistogramImageToImageMetric<
                                          FixedImageType,
                                          MovingImageType >;


  TransformType::Pointer      transform     = TransformType::New();
  OptimizerType::Pointer      optimizer     = OptimizerType::New();
  InterpolatorType::Pointer   interpolator  = InterpolatorType::New();
  RegistrationType::Pointer   registration  = RegistrationType::New();

  registration->SetOptimizer(     optimizer     );
  registration->SetTransform(     transform     );
  registration->SetInterpolator(  interpolator  );


  MetricType::Pointer metric = MetricType::New();
  registration->SetMetric( metric  );


  // Software Guide : BeginCodeSnippet
  using HistogramSizeType = MetricType::HistogramSizeType;

  HistogramSizeType  histogramSize;

  histogramSize.SetSize(2);

  histogramSize[0] = 256;
  histogramSize[1] = 256;

  metric->SetHistogramSize( histogramSize );

  // The Amoeba optimizer doesn't need the cost function to compute derivatives
  metric->ComputeGradientOff();
  // Software Guide : EndCodeSnippet


  const unsigned int numberOfParameters = transform->GetNumberOfParameters();


  using FixedImageReaderType = itk::ImageFileReader< FixedImageType  >;
  using MovingImageReaderType = itk::ImageFileReader< MovingImageType >;

  FixedImageReaderType::Pointer  fixedImageReader  = FixedImageReaderType::New();
  MovingImageReaderType::Pointer movingImageReader = MovingImageReaderType::New();

  fixedImageReader->SetFileName(  argv[1] );
  movingImageReader->SetFileName( argv[2] );

  registration->SetFixedImage(    fixedImageReader->GetOutput()    );
  registration->SetMovingImage(   movingImageReader->GetOutput()   );

  fixedImageReader->Update();
  movingImageReader->Update();

  FixedImageType::ConstPointer fixedImage = fixedImageReader->GetOutput();

  registration->SetFixedImageRegion( fixedImage->GetBufferedRegion() );


  transform->SetIdentity();

  using ParametersType = RegistrationType::ParametersType;

  ParametersType initialParameters =  transform->GetParameters();

  initialParameters[0] = 0.0;
  initialParameters[1] = 0.0;

  if( argc > 5 )
    {
    initialParameters[0] = std::stod( argv[4] );
    initialParameters[1] = std::stod( argv[5] );
    }

  registration->SetInitialTransformParameters( initialParameters  );

  std::cout << "Initial transform parameters = ";
  std::cout << initialParameters << std::endl;


  //  Software Guide : BeginLatex
  //
  //  The AmoebaOptimizer moves a simplex around the cost surface.  Here we set
  //  the initial size of the simplex (5 units in each of the parameters)
  //  Software Guide : EndLatex

  // Software Guide : BeginCodeSnippet
  OptimizerType::ParametersType simplexDelta( numberOfParameters );
  simplexDelta.Fill( 5.0 );

  optimizer->AutomaticInitialSimplexOff();
  optimizer->SetInitialSimplexDelta( simplexDelta );
  // Software Guide : EndCodeSnippet

  //  Software Guide : BeginLatex
  //
  //  The AmoebaOptimizer performs minimization by default. In this case
  //  however, the MutualInformation metric must be maximized. We should
  //  therefore invoke the \code{MaximizeOn()} method on the optimizer in order
  //  to set it up for maximization.
  //
  //  Software Guide : EndLatex

  // Software Guide : BeginCodeSnippet
  optimizer->MaximizeOn();
  // Software Guide : EndCodeSnippet


  //  Software Guide : BeginLatex
  //
  //  We also adjust the tolerances on the optimizer to define convergence.
  //  Here, we used a tolerance on the parameters of 0.1 (which will be one
  //  tenth of image unit, in this case pixels). We also set the tolerance on
  //  the cost function value to define convergence.  The metric we are using
  //  returns the value of Mutual Information.  So we set the function
  //  convergence to be 0.001 bits (bits are the appropriate units for
  //  measuring Information).
  //
  //  Software Guide : EndLatex

  // Software Guide : BeginCodeSnippet
  optimizer->SetParametersConvergenceTolerance( 0.1 );  // 1/10th pixel
  optimizer->SetFunctionConvergenceTolerance(0.001);    // 0.001 bits
  // Software Guide : EndCodeSnippet


  //  Software Guide : BeginLatex
  //  In the case where the optimizer never succeeds in reaching the desired
  //  precision tolerance, it is prudent to establish a limit on the number of
  //  iterations to be performed. This maximum number is defined with the
  //  method \code{SetMaximumNumberOfIterations()}.
  //
  //  \index{itk::Amoeba\-Optimizer!SetMaximumNumberOfIterations()}
  //
  //  Software Guide : EndLatex

  // Software Guide : BeginCodeSnippet
  optimizer->SetMaximumNumberOfIterations( 200 );
  // Software Guide : EndCodeSnippet


  // Create the Command observer and register it with the optimizer.
  //
  CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
  optimizer->AddObserver( itk::IterationEvent(), observer );


  try
    {
    registration->Update();
    std::cout << "Optimizer stop condition: "
              << registration->GetOptimizer()->GetStopConditionDescription()
              << std::endl;
    }
  catch( itk::ExceptionObject & err )
    {
    std::cout << "ExceptionObject caught !" << std::endl;
    std::cout << err << std::endl;
    return EXIT_FAILURE;
    }


  ParametersType finalParameters = registration->GetLastTransformParameters();

  const double finalTranslationX    = finalParameters[0];
  const double finalTranslationY    = finalParameters[1];

  double bestValue = optimizer->GetValue();


  // Print out results
  //
  std::cout << "Result = " << std::endl;
  std::cout << " Translation X = " << finalTranslationX  << std::endl;
  std::cout << " Translation Y = " << finalTranslationY  << std::endl;
  std::cout << " Metric value  = " << bestValue          << std::endl;


  using ResampleFilterType = itk::ResampleImageFilter<
                            MovingImageType,
                            FixedImageType >;

  TransformType::Pointer finalTransform = TransformType::New();

  finalTransform->SetParameters( finalParameters );
  finalTransform->SetFixedParameters( transform->GetFixedParameters() );

  ResampleFilterType::Pointer resample = ResampleFilterType::New();

  resample->SetTransform( finalTransform );
  resample->SetInput( movingImageReader->GetOutput() );


  resample->SetSize(    fixedImage->GetLargestPossibleRegion().GetSize() );
  resample->SetOutputOrigin(  fixedImage->GetOrigin() );
  resample->SetOutputSpacing( fixedImage->GetSpacing() );
  resample->SetOutputDirection( fixedImage->GetDirection() );
  resample->SetDefaultPixelValue( 100 );


  using OutputImageType = itk::Image< PixelType, Dimension >;

  using WriterType = itk::ImageFileWriter< OutputImageType >;

  WriterType::Pointer      writer =  WriterType::New();

  writer->SetFileName( argv[3] );

  writer->SetInput( resample->GetOutput() );
  writer->Update();

  // Software Guide : EndCodeSnippet

  return EXIT_SUCCESS;
}
