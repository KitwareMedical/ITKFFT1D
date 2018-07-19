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
#ifndef itkBlockMatchingMetricImageFilter_hxx
#define itkBlockMatchingMetricImageFilter_hxx

#include "itkBlockMatchingMetricImageFilter.h"

namespace itk
{
namespace BlockMatching
{

template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::MetricImageFilter() :
  m_FixedImageRegionDefined( false ),
  m_MovingImageRegionDefined( false )
{
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::SetFixedImage( FixedImageType * fixedImage )
{
  this->SetInput( 0, fixedImage );
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::SetMovingImage( MovingImageType * movingImage )
{
  this->SetInput( 1, movingImage );
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::SetFixedImageRegion( const FixedImageRegionType & region )
{
  FixedImageType * fixedPtr = const_cast< TFixedImage* >( this->GetInput(0) );
  if( !fixedPtr )
    {
    itkExceptionMacro( << "The FixedImage must be set before specifying the fixed image region." );
    }
  fixedPtr->UpdateOutputInformation();
  m_FixedImageRegion = region;
  if( !m_FixedImageRegion.Crop( fixedPtr->GetLargestPossibleRegion() ) )
    {
    itkExceptionMacro( << "Requested block is outside of the fixed image." );
    }
  typename FixedImageRegionType::SizeType fixedSize     = m_FixedImageRegion.GetSize();
  for( unsigned int i = 0; i < ImageDimension; ++i )
    {
    // The radius may have been truncated if the fixed region was outside the
    // fixed image's LargestPossibleRegion.
    if( fixedSize[i] % 2 == 0 )
      fixedSize[i]--;
    m_FixedRadius[i] = (fixedSize[i] - 1) / 2;
    }
  m_FixedImageRegion.SetSize( fixedSize );
  m_FixedImageRegionDefined = true;

  MovingImageType * movingPtr = const_cast< TMovingImage* >( this->GetInput(1) );
  if( !movingPtr )
    {
    itkExceptionMacro( << "The MovingImage must be set before specifying the fixed image region." );
    }
  movingPtr->UpdateOutputInformation();
  m_MovingRadius = m_FixedRadius;
  typename FixedImageType::SpacingType  fixedSpacing  = fixedPtr->GetSpacing();
  typename MovingImageType::SpacingType movingSpacing = movingPtr->GetSpacing();
  if( !( fixedSpacing == movingSpacing ) )
    {
    for( unsigned int i = 0; i < ImageDimension; ++i )
      {
      m_MovingRadius[i] = Math::Ceil< typename RadiusType::SizeValueType >(
      fixedSpacing[i] * m_FixedRadius[i] / movingSpacing[i] );
      }
    }
  this->Modified();
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::SetMovingImageRegion( const MovingImageRegionType & region )
{
  m_MovingImageRegion = region;
  m_MovingImageRegionDefined = true;
  this->Modified();
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::GenerateOutputInformation()
{
  const MovingImageType * movingPtr = this->GetInput( 1 );
  if( !movingPtr )
    {
    return;
    }

  MetricImageType * outputPtr = this->GetOutput();
  if( !outputPtr )
    {
    return;
    }

  if( !m_MovingImageRegionDefined )
    {
    itkExceptionMacro( << "MovingImageRegion has not been set" );
    }

  MetricImageRegionType                     metricRegion;
  typename MetricImageRegionType::IndexType metricIndex;
  metricIndex.Fill( 0 );
  metricRegion.SetIndex( metricIndex );
  // the Default is to to use the moving image size and spacing.
  metricRegion.SetSize( m_MovingImageRegion.GetSize() );
  outputPtr->SetLargestPossibleRegion( metricRegion );
  outputPtr->SetSpacing( movingPtr->GetSpacing() );

  typename MetricImageType::IndexType metricStart( m_MovingImageRegion.GetIndex() );

  typename MetricImageType::PointType origin;
  movingPtr->TransformIndexToPhysicalPoint( metricStart, origin );
  outputPtr->SetOrigin( origin );

  // The metric image direction is the same as the fixed image direction.
  outputPtr->SetDirection( movingPtr->GetDirection() );
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();

  // const cast so we can set the requested region.
  FixedImageType * fixedPtr = const_cast< TFixedImage* >( this->GetInput(0) );
  if( !fixedPtr )
    {
    return;
    }

  MovingImageType * movingPtr = const_cast< TMovingImage* >( this->GetInput(1) );
  if( !movingPtr )
    {
    return;
    }

  if( !m_FixedImageRegionDefined )
    {
    itkExceptionMacro( << "FixedImageRegion has not been set" );
    }

  if( !m_MovingImageRegionDefined )
    {
    itkExceptionMacro( << "MovingImageRegion has not been set" );
    }

  fixedPtr->SetRequestedRegion( m_FixedImageRegion );
  MovingImageRegionType movingImageRequestedRegion = m_MovingImageRegion;
  movingImageRequestedRegion.PadByRadius( m_MovingRadius );
  // make sure the requested region is within the largest possible.
  if ( movingImageRequestedRegion.Crop( movingPtr->GetLargestPossibleRegion() ) )
    {
    movingPtr->SetRequestedRegion( movingImageRequestedRegion );
    return;
    }
  else
    {
    // store what we tried( prior to try to crop )
    movingPtr->SetRequestedRegion( movingImageRequestedRegion );

    itkExceptionMacro(<< "Moving image requested region is at least partially outside the LargestPossibleRegion.");
    }
}


template< typename TFixedImage, typename TMovingImage, typename TMetricImage >
void
MetricImageFilter< TFixedImage, TMovingImage, TMetricImage >
::EnlargeOutputRequestedRegion(DataObject *data)
{
  Superclass::EnlargeOutputRequestedRegion( data );
  data->SetRequestedRegionToLargestPossibleRegion();
}

} // end namespace BlockMatching
} // end namespace itk

#endif
