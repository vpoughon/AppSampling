//#ifndef __otbPolygonImageFootprint_

#include <map>
#include "otbPersistentImageFilter.h"
#include "otbOGRDataSourceWrapper.h"

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
  PolygonImageFootprintFilter() {}

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
  otb::org::Layer ApplyPolygonsSpatialFilter(const TInputImage& image, const typename Self::OutputImageRegionType& threadRegion)
  {
    ImageType::IndexType urIndex;
    ImageType::IndexType llIndex;
      
    // ogr method for image region -> spatial filter convertion?
    // ex: SetSpatialFilterRegion(region)
    
    urIndex[0] = startX + sizeX;
    urIndex[1] = startY + sizeY;
    llIndex[0] = startX;
    llIndex[1] = startY;        
        
    itk::Point<double, 2> ulPoint;
    itk::Point<double, 2> lrPoint;
        
    image->TransformIndexToPhysicalPoint(urIndex, lrPoint);
    image->TransformIndexToPhysicalPoint(llIndex, ulPoint);  
        
    otb::ogr::Layer filtered = m_polygons->GetLayer(0);
    filtered.SetSpatialFilterRect(ulPoint[0], ulPoint[1], lrPoint[0], lrPoint[1]);         
        
    return filtered;
  }

  void ThreadedGenerateData(const typename Self::OutputImageRegionType& threadRegion, itk::ThreadIdType)
  {
    // Enable progress reporting
    //itk::ProgressReporter progress(this, threadId, threadRegion.GetNumberOfPixels());

    // Retrieve inputs
    // const cast is nominal apparently
    TInputImage* input_image = const_cast<TInputImage*>(this->GetInput());

    // This filter only needs the image metadata
    input_image->UpdateOutputInformation(); 

    // Consider only polygons in threadRegion
    otb::ogr::Layer filteredPolygons = ApplyPolygonsSpatialFilter(threadRegion);

    // Make iterator for image over threadRegion
    // Loop across the features in the layer
      // Loop accross pixels in the polygon and do stats
      // Count number of pixels in polygon
      // Add count to class total
      // Add count to total number of pixels

    // use otb::ogr::DataSource for shape file
  }

private:
  typename TInputMask::Pointer m_mask;
  otb::ogr::DataSource::Pointer m_polygons;

  // Temporary statistics
  std::vector<PolygonImageFootprintStatistics::Pointer> m_temporaryPolygonStatistics;

  // Final result
  PolygonImageFootprintStatistics::Pointer m_resultPolygonStatistics;
};

} // namespace otb

//#include "otbPolygonImageFootprint.txx"

