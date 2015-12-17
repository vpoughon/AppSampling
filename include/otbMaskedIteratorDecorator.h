#ifndef __otbMaskedIteratorDecorator_h
#define __otbMaskedIteratorDecorator_h

#include <boost/move/utility_core.hpp>

namespace otb
{

template <typename TIteratorType>
class MaskedIteratorDecorator
{
public:
  typedef MaskedIteratorDecorator<TIteratorType> Self;
  typedef typename TIteratorType::ImageType      MaskType;
  typedef typename TIteratorType::ImageType      ImageType;
  typedef typename ImageType::IndexType          IndexType;

  template <typename T1>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1):
                            m_mask(mask),
                            m_image(image),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1)),
                            m_itImage(image,
                                      boost::forward<T1>(arg1))
  {
    ComputeMaskedBegin();
  }

  template <typename T1,
            typename T2>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1,
                          BOOST_FWD_REF(T2) arg2):
                            m_mask(mask),
                            m_image(image),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1),
                                     boost::forward<T2>(arg2)),
                            TIteratorType(image,
                                          boost::forward<T1>(arg1),
                                          boost::forward<T2>(arg2))
  {
    ComputeMaskedBegin();
  }

  template <typename T1,
            typename T2,
            typename T3>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1,
                          BOOST_FWD_REF(T2) arg2,
                          BOOST_FWD_REF(T3) arg3):
                            m_mask(mask),
                            m_image(image),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1),
                                     boost::forward<T2>(arg2),
                                     boost::forward<T3>(arg3)),
                            TIteratorType(image,
                                          boost::forward<T1>(arg1),
                                          boost::forward<T2>(arg2),
                                          boost::forward<T2>(arg3))
  {
    ComputeMaskedBegin();
  }

  IndexType GetIndex() const
  {
    return m_itMask.GetIndex();
  }

  void GoToBegin()
  {
    m_itMask.SetIndex(m_begin);
    m_itImage.SetIndex(m_begin);
  }

  void GoToEnd()
  {
    m_itMask.GoToEnd();
    m_itImage.GoToEnd();
  }

  bool IsAtBegin() const
  {
    return m_itMask.GetIndex() == m_begin || m_itImage.GetIndex() == m_begin;
  }

  bool IsAtEnd() const
  {
    return m_itMask.IsAtEnd() || m_itImage.IsAtEnd();
  }

  // Wrap the underlying iterators to skip masked pixels
  Self& operator++()
  {
    do
    {
      ++m_itMask;
      ++m_itImage;
    } while (m_itMask.Value() == 0 && !this->IsAtEnd());
    return *this;
  }

  // Wrap the underlying iterators to skip masked pixels
  Self & operator--()
  {
    do
    {
      --m_itMask;
      --m_itImage;
    } while (m_itMask.Value() == 0 && !this->IsAtBegin());
    return *this;
  }

private:
  // Compute begin iterator position taking into account the mask
  void ComputeMaskedBegin()
  {
    // Start at the iterator pair actual begin
    m_itMask.GoToBegin();
    m_itImage.GoToEnd();

    // Advance to the first non-masked position, or the end
    while (m_itMask.Value() == 0 && !m_itMask.IsAtEnd() && !m_itImage.IsAtEnd())
    {
      ++m_itMask;
      ++m_itImage;
    }
    m_begin = m_itMask.GetIndex();
  }

private:
  // Pointers to the image and the mask
  typename MaskType::Pointer m_mask;
  typename ImageType::Pointer m_image;

  // Current position
  TIteratorType m_itMask;
  TIteratorType m_itImage;

  // Unmasked bounds
  IndexType m_begin;
};

}

#endif
