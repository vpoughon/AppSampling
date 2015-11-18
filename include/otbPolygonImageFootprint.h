#ifndef __otbPolygonImageFootprint__
#define __otbPolygonImageFootprint__

#include <map>
#include "otbPersistentImageFilter.h"
#include "otbOGRDataSourceWrapper.h"
#include "itkImageRegionIterator.h"
#include <iostream>

namespace otb
{

class PolygonImageFootprintStatistics : public itk::Object
{
public:
  typedef PolygonImageFootprintStatistics Self;
  typedef itk::SmartPointer<Self> Pointer;
  itkNewMacro(Self);

protected:
  PolygonImageFootprintStatistics() {}

private:
  //Number of pixels in all the polygons
  int nbPixelsGlobal; 
  //Number of pixels in each classes
  std::map<int, int> m_elmtsInClass;
  //Number of pixels in each polygons
  std::map<unsigned long, int> m_polygon;
  //Counter of pixels in the current polygon
  std::map<unsigned long, int> m_counterPixelsInPolygon;
  //Shifted counter of pixels in the current polygon
  std::map<unsigned long, int> m_counterPixelsInPolygonShifted;
  //RandomPosition of a sampled pixel for each polygons
  std::map<unsigned long, int> m_randomPositionInPolygon;

  // Not implemented
  PolygonImageFootprintStatistics(const Self&);
  void operator=(const Self&);
};

// how to make input mask optional wrt template params?
// set a default type?
template <class TInputImage, class TInputMask>
class ITK_EXPORT PolygonImageFootprintFilter :
  public otb::PersistentImageFilter<TInputImage, TInputImage>
{
public:
  typedef PolygonImageFootprintFilter<TInputImage, TInputMask> Self;
  typedef otb::PersistentImageFilter<TInputImage, TInputImage> Superclass;
  typedef itk::SmartPointer<Self> Pointer;

  itkNewMacro(Self);
  itkTypeMacro(Self, PersistentImageFilter);

  void SetPolygons(otb::ogr::DataSource::Pointer polygons)
  {
    this->m_polygons = polygons;
  }

  void SetMask(typename TInputMask::Pointer inputMask) // optional
  {
    this->m_mask = inputMask;
  }

  void setLayerIndex(vcl_size_t index)
  {
    m_layerIndex = index;
  }

  void GetResult()
  {
  }

public: // Software guide says this should be protected, but it won't compile
  PolygonImageFootprintFilter() :
    m_layerIndex(0)
  {
  }

  virtual ~PolygonImageFootprintFilter() {}

  void Reset()
  {
    unsigned int numberOfThreads = this->GetNumberOfThreads();
    
    // Reset list of individual containers
    m_temporaryPolygonStatistics = std::vector<PolygonImageFootprintStatistics::Pointer>(numberOfThreads);
    std::vector<PolygonImageFootprintStatistics::Pointer>::iterator it = m_temporaryPolygonStatistics.begin();
    for (; it != m_temporaryPolygonStatistics.end(); it++)
    {
      *it = PolygonImageFootprintStatistics::New();
    }
    m_resultPolygonStatistics = PolygonImageFootprintStatistics::New();
  }

  virtual void Synthetize() {}

protected:

  // Internal method for filtering polygons inside the current requested region
  void ApplyPolygonsSpatialFilter(const TInputImage* image, const typename Self::OutputImageRegionType& requestedRegion)
  {
    typename TInputImage::IndexType lowerIndex = requestedRegion.GetIndex();
    typename TInputImage::IndexType upperIndex = requestedRegion.GetUpperIndex();

    itk::Point<double, 2> lowerPoint;
    itk::Point<double, 2> upperPoint;

    image->TransformIndexToPhysicalPoint(upperIndex, upperPoint);
    image->TransformIndexToPhysicalPoint(lowerIndex, lowerPoint);  

    m_polygons->GetLayer(m_layerIndex).SetSpatialFilterRect(lowerPoint[0], lowerPoint[1], upperPoint[0], upperPoint[1]);         
  }

  void BeforeThreadedGenerateData()
  {
    TInputImage* inputImage = const_cast<TInputImage*>(this->GetInput());
    const typename TInputImage::RegionType& requestedRegion = inputImage->GetRequestedRegion();
    ApplyPolygonsSpatialFilter(inputImage, requestedRegion);
  }
  
  // ImageRegion containing the polygon feature
  // This should really be otb::ogr::Feature::GetEnvelope()
  //                   and otb::Image::EnvelopeToRegion()
  typename TInputImage::RegionType FeatureBoundingRegion(const TInputImage* image, const otb::ogr::Feature& feature) const
  {
    // otb::ogr wrapper is incomplete and leaky abstraction is inevitable here
    OGRGeometry* geom = feature.ogr().GetGeometryRef();

    // Bounding envelope of feature
    OGREnvelope envelope;
    geom->getEnvelope(&envelope);
    
    itk::Point<double, 2> lowerPoint, upperPoint;
    lowerPoint[0] = envelope.MinX;
    lowerPoint[1] = envelope.MinY;
    upperPoint[0] = envelope.MaxX;
    upperPoint[1] = envelope.MaxY;

    // Convert to ImageRegion
    typename TInputImage::IndexType lowerIndex;
    typename TInputImage::IndexType upperIndex;

    image->TransformPhysicalPointToIndex(lowerPoint, lowerIndex);
    image->TransformPhysicalPointToIndex(upperPoint, upperIndex);

    typename TInputImage::RegionType region;
    region.SetIndex(lowerIndex);
    region.SetUpperIndex(upperIndex);
    return region;
  }

  void ThreadedGenerateData(const typename Self::OutputImageRegionType& threadRegion, itk::ThreadIdType)
  {
    // Retrieve inputs
    // const cast is nominal apparently
    // raw pointer?
    TInputImage* inputImage = const_cast<TInputImage*>(this->GetInput());

    // Loop across the features in the layer (filtered by requested region in BeforeTGD already)
    otb::ogr::Layer::const_iterator featIt = m_polygons->GetLayer(m_layerIndex).begin(); 
    for(; featIt!=m_polygons->GetLayer(m_layerIndex).end(); featIt++)
    {
      // Compute the considered region (optimization)
      // i.e. the intersection of thread region and polygon's bounding box
      // This need not be done in ThreadedGenerateData and could be
      // pre-processed and cached before filter execution if needed
      typename TInputImage::RegionType consideredRegion = FeatureBoundingRegion(inputImage, *featIt);
      bool regionNotEmpty = consideredRegion.Crop(threadRegion);

      if (regionNotEmpty)
      {
        // For pixels in consideredRegion
        itk::ImageRegionIterator<TInputImage> itImage(inputImage, consideredRegion);
        itk::ImageRegionIterator<TInputImage> itMask;
        if (m_mask)
        {
          itMask = itk::ImageRegionIterator<TInputImage>(m_mask, consideredRegion);
        }
        for (itImage.GoToBegin(); !itImage.IsAtEnd(); ++itImage)
        {
          if (m_mask)
          {
            ++itMask;
          }
          // Test if pixel is in mask
          if (m_mask && itMask.Value() == 1)
          {
            itk::Point<double, 2> point;
            inputImage->TransformIndexToPhysicalPoint(itImage.GetIndex(), point);
            // Test if point is in feature
            // ->Test if the current pixel is in a polygon hole
            // Count number of pixels in current feature
            // Add count to class total
            // Add count to total number of pixels
            //nbOfPixelsInGeom++;
            //nbPixelsGlobal++;
          }
        }

        //Class name recuperation
        //int className = featIt->ogr().GetFieldAsInteger(GetParameterString("cfield").c_str());

        //Counters update, number of pixel in each classes and in each polygons
        //polygon[featIt->ogr().GetFID()] += nbOfPixelsInGeom;

        //Generation of a random number for the sampling in a polygon where we only need one pixel, it's choosen randomly
        //elmtsInClass[className] = elmtsInClass[className] + nbOfPixelsInGeom;
      }
    }

    // use otb::ogr::DataSource for shape file
  }

  void AfterThreadedGenerateData()
  {
  }

private:
  typename TInputMask::Pointer m_mask;

  // Polygons layer of the current tile
  // TODO this could be a second itk::DataObject input to the filter
  otb::ogr::DataSource::Pointer m_polygons;

  // Temporary statistics
  std::vector<PolygonImageFootprintStatistics::Pointer> m_temporaryPolygonStatistics;

  // Final result
  PolygonImageFootprintStatistics::Pointer m_resultPolygonStatistics;

  // Layer to use in the shape file, default to 0
  vcl_size_t m_layerIndex;
};

} // namespace otb

//#include "otbPolygonImageFootprint.txx"

#endif
