#ifndef __itkAnalyticSignalImageFilter_hxx
#define __itkAnalyticSignalImageFilter_hxx

#include "itkAnalyticSignalImageFilter.h"

#include "itkVnlFFT1DRealToComplexConjugateImageFilter.h"
#include "itkVnlFFT1DComplexToComplexImageFilter.h"

#if defined(USE_FFTWD) || defined(USE_FFTWF)
#include "itkFFTW1DRealToComplexConjugateImageFilter.h"
#include "itkFFTW1DComplexToComplexImageFilter.h"
#endif

#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkImageLinearIteratorWithIndex.h"
#include "itkMetaDataObject.h"

namespace itk
{

template < class TPixel, unsigned int VDimension >
AnalyticSignalImageFilter< TPixel, VDimension >
::AnalyticSignalImageFilter()
{
  m_FFTRealToComplexFilter = FFTRealToComplexType::New();
  m_FFTComplexToComplexFilter = FFTComplexToComplexType::New();

  m_FFTComplexToComplexFilter->SetTransformDirection( FFTComplexToComplexType::INVERSE );

  this->SetDirection( 0 );

  this->m_ImageRegionSplitter = ImageRegionSplitterDirection::New();
}

template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the inputs
  typename InputImageType::Pointer inputPtr  =
    const_cast<InputImageType *> (this->GetInput());
  typename OutputImageType::Pointer outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // we need to compute the input requested region (size and start index)
  typedef const typename OutputImageType::SizeType& OutputSizeType;
  OutputSizeType outputRequestedRegionSize =
    outputPtr->GetRequestedRegion().GetSize();
  typedef const typename OutputImageType::IndexType& OutputIndexType;
  OutputIndexType outputRequestedRegionStartIndex =
    outputPtr->GetRequestedRegion().GetIndex();

  //// the regions other than the fft direction are fine
  typename InputImageType::SizeType  inputRequestedRegionSize = outputRequestedRegionSize;
  typename InputImageType::IndexType inputRequestedRegionStartIndex = outputRequestedRegionStartIndex;

  // we but need all of the input in the fft direction
  const unsigned int direction = this->GetDirection();
  const typename InputImageType::SizeType& inputLargeSize =
    inputPtr->GetLargestPossibleRegion().GetSize();
  inputRequestedRegionSize[direction] = inputLargeSize[direction];
  const typename InputImageType::IndexType& inputLargeIndex =
    inputPtr->GetLargestPossibleRegion().GetIndex();
  inputRequestedRegionStartIndex[direction] = inputLargeIndex[direction];

  typename InputImageType::RegionType inputRequestedRegion;
  inputRequestedRegion.SetSize( inputRequestedRegionSize );
  inputRequestedRegion.SetIndex( inputRequestedRegionStartIndex );

  inputPtr->SetRequestedRegion( inputRequestedRegion );
}


template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::EnlargeOutputRequestedRegion(DataObject *output)
{
  OutputImageType* outputPtr = dynamic_cast< OutputImageType* >( output );

  // we need to enlarge the region in the fft direction to the
  // largest possible in that direction
  typedef const typename OutputImageType::SizeType& ConstOutputSizeType;
  ConstOutputSizeType requestedSize =
    outputPtr->GetRequestedRegion().GetSize();
  ConstOutputSizeType outputLargeSize =
    outputPtr->GetLargestPossibleRegion().GetSize();
  typedef const typename OutputImageType::IndexType& ConstOutputIndexType;
  ConstOutputIndexType requestedIndex =
    outputPtr->GetRequestedRegion().GetIndex();
  ConstOutputIndexType outputLargeIndex =
    outputPtr->GetLargestPossibleRegion().GetIndex();

  typename OutputImageType::SizeType enlargedSize   = requestedSize;
  typename OutputImageType::IndexType enlargedIndex = requestedIndex;
  const unsigned int direction = this->GetDirection ();
  enlargedSize[direction]  = outputLargeSize[direction];
  enlargedIndex[direction] = outputLargeIndex[direction];

  typename OutputImageType::RegionType enlargedRegion;
  enlargedRegion.SetSize( enlargedSize );
  enlargedRegion.SetIndex( enlargedIndex );
  outputPtr->SetRequestedRegion( enlargedRegion );
}


template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );

  const unsigned int direction = this->GetDirection();
  os << indent << "Direction: " << direction << std::endl;

  os << indent << "FFTRealToComplexFilter: " << std::endl;
  m_FFTRealToComplexFilter->Print( os, indent );
  os << indent << "FFTComplexToComplexFilter: " << std::endl;
  m_FFTComplexToComplexFilter->Print( os, indent );
}


template <class TPixel, unsigned int VDimension>
const ImageRegionSplitterBase*
AnalyticSignalImageFilter< TPixel, VDimension >
::GetImageRegionSplitter(void) const
{
  return this->m_ImageRegionSplitter.GetPointer();
}


template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::BeforeThreadedGenerateData()
{
  this->m_ImageRegionSplitter->SetDirection( this->GetDirection() );

  m_FFTRealToComplexFilter->SetInput( this->GetInput() );
  m_FFTRealToComplexFilter->GetOutput()->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
  m_FFTRealToComplexFilter->GetOutput()->SetLargestPossibleRegion( this->GetOutput()->GetLargestPossibleRegion() );
  m_FFTRealToComplexFilter->SetNumberOfThreads( this->GetNumberOfThreads() );
  m_FFTRealToComplexFilter->Update ();
}

template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
  ThreadIdType threadId )
{
  // get pointers to the input and output
  typename FFTRealToComplexType::OutputImageType::ConstPointer  inputPtr  = m_FFTRealToComplexFilter->GetOutput();
  typename OutputImageType::Pointer				outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  const typename FFTRealToComplexType::OutputImageType::SizeType&   inputSize
    = inputPtr->GetRequestedRegion().GetSize();
  const unsigned int direction = this->GetDirection ();
  const unsigned int size = inputSize[direction];
  unsigned int dub_size;
  bool even;
  if( size % 2 == 0 )
    {
    even = true;
    dub_size = size / 2 - 1;
    }
  else
    {
    even = false;
    dub_size = (size + 1) / 2 - 1;
    }

  typedef itk::ImageLinearConstIteratorWithIndex< OutputImageType >  InputIteratorType;
  typedef itk::ImageLinearIteratorWithIndex< OutputImageType >      OutputIteratorType;
  InputIteratorType inputIt( inputPtr, outputRegionForThread );
  OutputIteratorType outputIt( outputPtr, outputRegionForThread );
  inputIt.SetDirection( direction );
  outputIt.SetDirection( direction );

  unsigned int i;
  // for every fft line
  for( inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd();
    outputIt.NextLine(), inputIt.NextLine() )
    {
    inputIt.GoToBeginOfLine();
    outputIt.GoToBeginOfLine();

    // DC
    outputIt.Set( inputIt.Get() );
    ++inputIt;
    ++outputIt;

    for( i = 0; i < dub_size; i++ )
      {
      outputIt.Set( inputIt.Get() * static_cast< TPixel >( 2 ) );
      ++outputIt;
      ++inputIt;
      }
    if( even )
      {
      outputIt.Set( inputIt.Get() );
      ++inputIt;
      ++outputIt;
      }
    while( !outputIt.IsAtEndOfLine() )
      {
      outputIt.Set( static_cast< TPixel >( 0 ) );
      ++outputIt;
      }
    }
}

template < class TPixel , unsigned int Dimension >
void
AnalyticSignalImageFilter < TPixel , Dimension >
::AfterThreadedGenerateData()
{
  // Trippy, eh?
  m_FFTComplexToComplexFilter->SetInput( this->GetOutput() );
  m_FFTComplexToComplexFilter->GetOutput()->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
  m_FFTComplexToComplexFilter->GetOutput()->SetLargestPossibleRegion( this->GetOutput()->GetLargestPossibleRegion() );
  m_FFTComplexToComplexFilter->SetNumberOfThreads( this->GetNumberOfThreads() );
  m_FFTComplexToComplexFilter->Update ();
  this->GraftOutput( m_FFTComplexToComplexFilter->GetOutput() );
}

} // end namespace itk

#endif // __itkAnalyticSignalImageFilter_hxx
