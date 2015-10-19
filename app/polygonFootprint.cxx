/*=========================================================================
 Program:   ORFEO Toolbox
 Language:  C++
 Date:      $Date$
 Version:   $Revision$


 Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
 See OTBCopyright.txt for details.


 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 =========================================================================*/

/*
 * The PolygonFootprint application exposes the PolygonImageFootprint filter
 * Inputs:
 *   - The image
 *   - The shapefile
 *   - Optionally a mask indicating which input image pixels are to be considered
 *   -
 * Outputs:
 *   -
 *   -
 */

#include "otbImage.h"
#include "otbPolygonImageFootprint.h"
#include "otbWrapperApplication.h"
#include "otbWrapperApplicationFactory.h"
#include "otbImageFileReader.h"
#include "otbPersistentFilterStreamingDecorator.h"

namespace otb
{

namespace Wrapper
{
  
class PolygonFootprint : public Application
{
public:
  typedef PolygonFootprint Self;
  typedef itk::SmartPointer<Self> Pointer; 
  itkNewMacro(Self);
  itkTypeMacro(PolygonFootprint, otb::Application);
  
private:
  void DoInit()
  {
    SetName("PolygonFootprints");
    SetDescription("");
    
    AddParameter(ParameterType_InputFilename, "image", "Input image");    
    AddParameter(ParameterType_InputFilename, "shapefile", "Input shapefile");    
    AddParameter(ParameterType_InputImage, "mask", "Input mask (optional)");
    MandatoryOff("mask");

    AddParameter(ParameterType_OutputFilename, "out", "Output XML file");       
    AddParameter(ParameterType_String, "cfield", "Field of class");
  }

  void DoUpdateParameters() {}

  void DoExecute()
  {  
    typedef otb::Image<float, 2> ImageType;
    //typedef otb::Image<unsigned char, 2> MaskType;

    // Reader
    typedef otb::ImageFileReader<ImageType> ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(GetParameterString("image").c_str());

    // Input shape file
    otb::ogr::DataSource::Pointer polygons = otb::ogr::DataSource::New(GetParameterString("shapefile").c_str(), otb::ogr::DataSource::Modes::Read);

    // Input mask
    //

    typedef otb::PersistentFilterStreamingDecorator < PolygonImageFootprintFilter<ImageType, ImageType> > FilterType;
    FilterType::Pointer filter = FilterType::New();

    filter->GetFilter()->SetInput(reader->GetOutput());
    filter->GetFilter()->SetPolygons(polygons);
    // if provided
      // filter.SetMask(

    filter->Update();
  }
};
}
}

OTB_APPLICATION_EXPORT(otb::Wrapper::PolygonFootprint)
