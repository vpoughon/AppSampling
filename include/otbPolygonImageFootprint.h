//#ifndef __otbPolygonImageFootprint_

#include <map>
#include "otbPersistentImageFilter.h"
#include "otbOGRDataSourceWrapper.h"
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

  void GetResult()
  {
  }

public: // Software guide says this should be protected, but it won't compile
  PolygonImageFootprintFilter() {
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

  virtual void Synthetize()
  {
  }

protected:

  // Internal method for filtering polygons inside the current thread region
  void ApplyPolygonsSpatialFilter(const TInputImage* image, const typename Self::OutputImageRegionType& requestedRegion)
  {
    typename TInputImage::IndexType lowerIndex = requestedRegion.GetIndex();
    typename TInputImage::IndexType upperIndex = requestedRegion.GetUpperIndex();

    itk::Point<double, 2> lowerPoint;
    itk::Point<double, 2> upperPoint;

    image->TransformIndexToPhysicalPoint(upperIndex, upperPoint);
    image->TransformIndexToPhysicalPoint(lowerIndex, lowerPoint);  

    // TODO add parameter to choose layer number, don't hardcode 0
    m_polygons->GetLayer(0).SetSpatialFilterRect(lowerPoint[0], lowerPoint[1], upperPoint[0], upperPoint[1]);         
  }

  void BeforeThreadedGenerateData()
  {
    TInputImage* inputImage = const_cast<TInputImage*>(this->GetInput());
    const typename TInputImage::RegionType& requestedRegion = inputImage->GetRequestedRegion();
    ApplyPolygonsSpatialFilter(inputImage, requestedRegion);
  }

  void ThreadedGenerateData(const typename Self::OutputImageRegionType& threadRegion, itk::ThreadIdType)
  {
    // Enable progress reporting
    //itk::ProgressReporter progress(this, threadId, threadRegion.GetNumberOfPixels());

    // Retrieve inputs
    // const cast is nominal apparently
    // raw pointer?
    TInputImage* input_image = const_cast<TInputImage*>(this->GetInput());

    // Make iterator for image over threadRegion
    // Loop across the features in the layer
      // Loop accross pixels in the polygon and do stats
      // Count number of pixels in polygon
      // Add count to class total
      // Add count to total number of pixels

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

};

} // namespace otb

//#include "otbPolygonImageFootprint.txx"

