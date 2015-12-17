#include "itkImageRegionIterator.h"
#include "otbImage.h"
#include "otbMaskedIteratorDecorator.h"

int otbMaskedIteratorDecoratorNew(int itkNotUsed(argc), char * itkNotUsed(argv) [])
{
  typedef otb::Image<double, 2> ImageType;

  ImageType::Pointer image = ImageType::New();
  ImageType::Pointer mask = ImageType::New();
  ImageType::RegionType region(image->GetLargestPossibleRegion());

  otb::MaskedIteratorDecorator<itk::ImageRegionIterator<ImageType> > it(mask, image, region);
  return EXIT_SUCCESS;
}
