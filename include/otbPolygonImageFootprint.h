#ifndef __otbPolygonImageFootprint__
#define __otbPolygonImageFootprint__

#include <map>
#include "otbPersistentImageFilter.h"
#include "otbOGRDataSourceWrapper.h"
#include "otbMaskedIteratorDecorator.h"
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

  // Convert an OGR ImageRegion
  typename TInputImage::RegionType EnvelopeToRegion(const TInputImage* image, OGREnvelope envelope) const
  {
    itk::Point<double, 2> lowerPoint, upperPoint;
    lowerPoint[0] = envelope.MinX;
    lowerPoint[1] = envelope.MinY;
    upperPoint[0] = envelope.MaxX;
    upperPoint[1] = envelope.MaxY;

    typename TInputImage::IndexType lowerIndex;
    typename TInputImage::IndexType upperIndex;

    image->TransformPhysicalPointToIndex(lowerPoint, lowerIndex);
    image->TransformPhysicalPointToIndex(upperPoint, upperIndex);

    typename TInputImage::RegionType region;
    region.SetIndex(lowerIndex);
    region.SetUpperIndex(upperIndex);
    return region;
  }
  
  // ImageRegion containing the polygon feature
  typename TInputImage::RegionType FeatureBoundingRegion(const TInputImage* image, const otb::ogr::Feature& feature) const
  {
    // otb::ogr wrapper is incomplete and leaky abstraction is inevitable here
    OGREnvelope envelope;
    feature.GetGeometry()->getEnvelope(&envelope);
    return EnvelopeToRegion(image, envelope);
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
      // Optimization: compute the intersection of thread region and polygon bounding region,
      // called "considered region" here.
      // This need not be done in ThreadedGenerateData and could be
      // pre-processed and cached before filter execution if needed
      typename TInputImage::RegionType consideredRegion = FeatureBoundingRegion(inputImage, *featIt);
      bool regionNotEmpty = consideredRegion.Crop(threadRegion);

      otb::MaskedIteratorDecorator<itk::ImageRegionIterator<TInputImage> > it(m_mask, inputImage, consideredRegion);

      if (regionNotEmpty)
      {
        //OGRPolygon* inPolygon = dynamic_cast<OGRPolygon *>(geom);
        //OGRLinearRing* exteriorRing = inPolygon->getExteriorRing();
        // For pixels in unmasked consideredRegion
        for (it.GoToBegin(); !it.IsAtEnd(); ++it)
        {
          itk::Point<double, 2> point;
          inputImage->TransformIndexToPhysicalPoint(it.GetIndex(), point);
          // ->Test if the current pixel is in a polygon hole
          // If point is in feature
          //if(exteriorRing->isPointInRing(&pointOGR, TRUE) && isNotInHole)
          //{
            // Count
            //nbOfPixelsInGeom++;
            //nbPixelsGlobal++;
          //}
        }

        //Class name recuperation
        //int className = featIt->ogr().GetFieldAsInteger(GetParameterString("cfield").c_str());

        //Counters update, number of pixel in each classes and in each polygons
        //polygon[featIt->ogr().GetFID()] += nbOfPixelsInGeom;

        //Generation of a random number for the sampling in a polygon where we only need one pixel, it's choosen randomly
        //elmtsInClass[className] = elmtsInClass[className] + nbOfPixelsInGeom;
      }
    }
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
