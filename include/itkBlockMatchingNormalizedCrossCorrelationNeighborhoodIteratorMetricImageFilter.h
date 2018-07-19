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
#ifndef itkBlockMatchingNormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter_h
#define itkBlockMatchingNormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter_h

#include "itkBlockMatchingNormalizedCrossCorrelationMetricImageFilter.h"

namespace itk
{
namespace BlockMatching
{

/** \class NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter
 *
 * \brief Create an image of the the normalized cross correlation with a kernel
 * calculated with a neighborhood iterator.
 *
 * \sa NormalizedCrossCorrelationMetricImageFilter
 *
 * \ingroup Ultrasound
 */
template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
class ITK_TEMPLATE_EXPORT NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter :
  public NormalizedCrossCorrelationMetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
{
public:
  ITK_DISALLOW_COPY_AND_ASSIGN(NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter);

  /** Standard class typedefs. */
  typedef NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter                        Self;
  typedef NormalizedCrossCorrelationMetricImageFilter< TFixedImage, TMovingImage, TMetricImage > Superclass;
  typedef SmartPointer<Self>                                                                     Pointer;
  typedef SmartPointer<const Self>                                                               ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter,
               NormalizedCrossCorrelationMetricImageFilter);

  /** ImageDimension enumeration. */
  itkStaticConstMacro(ImageDimension, unsigned int, TFixedImage::ImageDimension);

  /** Type of the fixed image. */
  typedef typename Superclass::FixedImageType   FixedImageType;

  /** Type of the moving image. */
  typedef typename Superclass::MovingImageType   MovingImageType;

  /** Type of the metric image. */
  typedef typename Superclass::MetricImageType   MetricImageType;
  typedef typename MetricImageType::RegionType   MetricImageRegionType;
  typedef typename MetricImageType::PixelType    MetricImagePixelType;

protected:
  NormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter(){}

  void BeforeThreadedGenerateData() override;

  void DynamicThreadedGenerateData( const MetricImageRegionType& outputRegion ) override;

private:
};

} // end namespace BlockMatching
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBlockMatchingNormalizedCrossCorrelationNeighborhoodIteratorMetricImageFilter.hxx"
#endif

#endif
