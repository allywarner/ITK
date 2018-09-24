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


#include "itkImageToRectilinearFEMObjectFilter.h"
#include "itkImageFileReader.h"
#include "itkFEMElement2DC0LinearQuadrilateralMembrane.h"
#include "itkMath.h"

int itkImageToRectilinearFEMObjectFilter2DTest(int argc, char *argv[])
{
  if(argc < 1)
    {
    std::cerr << "Missing Spatial Object Filename" << std::endl;
    return EXIT_FAILURE;
    }
  //Need to register default FEM object types,
  //and setup SpatialReader to recognize FEM types
  //which is all currently done as a HACK in
  //the initializaiton of the itk::FEMFactoryBase::GetFactory()
  itk::FEMFactoryBase::GetFactory()->RegisterDefaultTypes();

  using ImageType = itk::Image<unsigned char, 2>;
  using ImageFileReaderType = itk::ImageFileReader<ImageType>;
  double tolerance = 0.0001;

  vnl_vector<unsigned int> pixelsPerElement;
  vnl_vector<unsigned int> numberOfElements;
  pixelsPerElement.set_size(2);
  numberOfElements.set_size(2);
  pixelsPerElement[0] = static_cast<unsigned int>( std::stoi( argv[2] ) );
  pixelsPerElement[1] = static_cast<unsigned int>( std::stoi( argv[3] ) );
  numberOfElements[0] = static_cast<unsigned int>( std::stoi( argv[4] ) );
  numberOfElements[1] = static_cast<unsigned int>( std::stoi( argv[5] ) );

  ImageFileReaderType::Pointer reader = ImageFileReaderType::New();
  reader->SetFileName(argv[1]);
  reader->Update();

  /* Define the Material and Element Type to be used */
  using ElasticityType = itk::fem::MaterialLinearElasticity;
  ElasticityType::Pointer m;
  m = ElasticityType::New();
  m->SetGlobalNumber(0);
  m->SetYoungsModulus(3000.0);
  m->SetCrossSectionalArea(0.02);
  m->SetMomentOfInertia(0.004);

  using MembraneElementType = itk::fem::Element2DC0LinearQuadrilateralMembrane;
  MembraneElementType::Pointer e0 = MembraneElementType::New();
  e0->SetGlobalNumber(0);
  if ( dynamic_cast<ElasticityType *>( m.GetPointer() ))
    {
    e0->SetMaterial( dynamic_cast<ElasticityType *>( m.GetPointer() ) );
    }
  using MeshFilterType = itk::fem::ImageToRectilinearFEMObjectFilter<ImageType>;
  MeshFilterType::Pointer meshFilter = MeshFilterType::New();
  meshFilter->SetInput( reader->GetOutput() );
  meshFilter->SetPixelsPerElement( pixelsPerElement );
  meshFilter->SetElement( e0 );
  meshFilter->SetMaterial( m );
  meshFilter->Update();

  using FEMObjectType = itk::fem::FEMObject<2>;
  FEMObjectType::Pointer femObject = meshFilter->GetOutput();
  std::cout << "FEM Object Generation Test:";
  if( !femObject )
    {
    std::cout << " [FAILED]" << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << " [PASSED]" << std::endl;

  // Test the resulting FEMOBject
  bool               foundError = false;
  const auto expectedNumberOfNodes = static_cast<unsigned int>( std::stoi( argv[6] ) );
  const auto expectedNumberOfElements = static_cast<unsigned int>( std::stoi( argv[7] ) );

  vnl_vector<unsigned int> testPixelsPerElement = meshFilter->GetPixelsPerElement();
  vnl_vector<unsigned int> testNumberOfElements = meshFilter->GetNumberOfElements();
  for( unsigned int i = 0; i < 2; i++ )
    {
    std::cout << "Pixels per Element Test " << i << ":";
    if( testPixelsPerElement[i] != pixelsPerElement[i] )
      {
      std::cout << " [FAILED]" << std::endl;
      std::cout << "\tExpected " << pixelsPerElement[i] << " Obtained ";
      std::cout << testPixelsPerElement[i] << std::endl;
      foundError = true;
      }
    else
      {
      std::cout << " [PASSED]" << std::endl;
      }

    std::cout << "Number Of Elements Test " << i << ":";
    if( testNumberOfElements[i] != numberOfElements[i] )
      {
      std::cout << " [FAILED]" << std::endl;
      std::cout << "\tExpected " << numberOfElements[i] << " Obtained ";
      std::cout << testNumberOfElements[i] << std::endl;
      foundError = true;
      }
    else
      {
      std::cout << " [PASSED]" << std::endl;
      }

    }

  std::cout << "Number of Nodes Test :";
  if( femObject->GetNumberOfElements() != expectedNumberOfElements )
    {
    std::cout << "[FAILED]" << std::endl;
    std::cout << "\tExpected  " << expectedNumberOfElements << " Obtained ";
    std::cout << femObject->GetNumberOfElements() << std::endl;
    foundError = true;
    }
  else
    {
    std::cout << " [PASSED]" << std::endl;
    }

  std::cout << "Number of Element Test :";
  if( femObject->GetNumberOfNodes() != expectedNumberOfNodes )
    {
    std::cout << " [FAILED]" << std::endl;
    std::cout << "\tExpected  " << expectedNumberOfNodes << " Obtained ";
    std::cout << femObject->GetNumberOfNodes() << std::endl;
    foundError = true;
    }
  else
    {
    std::cout << " [PASSED]" << std::endl;
    }

  std::cout << "Number of Materials Test :";
  if ( femObject->GetNumberOfMaterials() != 1 )
    {
    std::cout << " [FAILED]" << std::endl;
    std::cout << "\tExpected  1" << " Obtained ";
    std::cout << femObject->GetNumberOfMaterials() << std::endl;
    foundError = true;
    }
  else
    {
    std::cout << " [PASSED]" << std::endl;
    }


  std::cout << "Material Property Test :";

  ElasticityType * m1 =
    dynamic_cast<itk::fem::MaterialLinearElasticity *>( femObject->GetMaterial(0).GetPointer() );
  if ( m1 == nullptr)
    {
    std::cout << " [FAILED]" << std::endl;
    std::cout << "\tdynamic_cast<itk::fem::MaterialLinearElasticity *>( femObject->GetMaterial(0).GetPointer() ) failed" << std::endl;
    foundError = true;
    }
  else if ((m1->GetYoungsModulus() != 3000.0) ||
           (itk::Math::NotExactlyEquals(m1->GetCrossSectionalArea(), 0.02)) ||
           (itk::Math::NotExactlyEquals(m1->GetMomentOfInertia(), 0.004)) )
    {
    std::cout << " [FAILED]" << std::endl;
    std::cout << "\tExpected  3000.0, 0.02, 0.004" << " Obtained ";
    std::cout << m1->GetYoungsModulus() << ", ";
    std::cout << m1->GetCrossSectionalArea() << ", ";
    std::cout << m1->GetMomentOfInertia() << std::endl;
    foundError = true;
    }
  else
    {
    std::cout << " [PASSED]" << std::endl;
   }

  const auto numberOfNodesToTest = static_cast<unsigned int>( std::stoi( argv[8] ) );
  for( unsigned int i = 0; i < numberOfNodesToTest; i++ )
    {
    auto nodeNumber = static_cast<unsigned int>( std::stoi( argv[9 + i * 3] ) );
    vnl_vector<double> loc;
    loc.set_size(2);
    loc[0] = std::stod( argv[9 + i * 3 + 1] );
    loc[1] = std::stod( argv[9 + i * 3 + 2] );
    std::cout << "Node (" << nodeNumber << ") Test " << i << ": ";
    if( ( std::fabs(femObject->GetNode(nodeNumber)->GetCoordinates()[0] - loc[0]) > tolerance) ||
        ( std::fabs(femObject->GetNode(nodeNumber)->GetCoordinates()[1] - loc[1]) > tolerance) )
      {
      std::cout << "[FAILED]" << std::endl;
      std::cout << "\tExpected (" << loc[0] << "," << loc[1] << "), Got (";
      std::cout << femObject->GetNode(nodeNumber)->GetCoordinates()[0] << ",";
      std::cout << femObject->GetNode(nodeNumber)->GetCoordinates()[1] << ")" << std::endl;
      foundError = true;
      }
    else
      {
      std::cout << "[PASSED]" << std::endl;
      }

    }

  const auto numberOfElementsToTest = static_cast<unsigned int>( std::stoi( argv[9 + numberOfNodesToTest * 3] ) );
  for( unsigned int i = 0; i < numberOfElementsToTest; i++ )
    {
    auto elementNumber = static_cast<unsigned int>( std::stoi( argv[10 + numberOfNodesToTest * 3 + i * 5] ) );
    vnl_vector<int> nodes;
    nodes.set_size(4);
    nodes[0] = std::stoi( argv[10 + numberOfNodesToTest * 3 + i * 5 + 1] );
    nodes[1] = std::stoi( argv[10 + numberOfNodesToTest * 3 + i * 5 + 2] );
    nodes[2] = std::stoi( argv[10 + numberOfNodesToTest * 3 + i * 5 + 3] );
    nodes[3] = std::stoi( argv[10 + numberOfNodesToTest * 3 + i * 5 + 4] );

    std::cout << "Element (" << elementNumber << ") Test " << i << ": ";
    if( (femObject->GetElement(elementNumber)->GetNode(0)->GetGlobalNumber() != nodes[0]) ||
        (femObject->GetElement(elementNumber)->GetNode(1)->GetGlobalNumber() != nodes[1]) ||
        (femObject->GetElement(elementNumber)->GetNode(2)->GetGlobalNumber() != nodes[2]) ||
        (femObject->GetElement(elementNumber)->GetNode(3)->GetGlobalNumber() != nodes[3]) )
      {
      std::cout << "[FAILED]" << std::endl;
      std::cout << "\tExpected (" << nodes[0] << "," << nodes[0] << "," << nodes[1];
      std::cout << "," << nodes[2] << "," << nodes[3] << "), Got (";
      std::cout << femObject->GetElement(elementNumber)->GetNode(0)->GetGlobalNumber() << ",";
      std::cout << femObject->GetElement(elementNumber)->GetNode(1)->GetGlobalNumber() << ",";
      std::cout << femObject->GetElement(elementNumber)->GetNode(2)->GetGlobalNumber() << ",";
      std::cout << femObject->GetElement(elementNumber)->GetNode(3)->GetGlobalNumber() << ")" << std::endl;
      foundError = true;
      }
    else
      {
      std::cout << "[PASSED]" << std::endl;
      }
    }

  if( foundError )
    {
    std::cout << "Test FAILED!" << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "Test PASSED!" << std::endl;
  return EXIT_SUCCESS;
}
