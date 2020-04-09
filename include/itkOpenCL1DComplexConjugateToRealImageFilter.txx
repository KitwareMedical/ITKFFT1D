#ifndef __itkOpenCL1DComplexConjugateToRealImageFilter_txx
#define __itkOpenCL1DComplexConjugateToRealImageFilter_txx

#include "itkFFT1DComplexConjugateToRealImageFilter.txx"
#include "itkOpenCL1DComplexConjugateToRealImageFilter.h"

#include <vector>

#include "itkIndent.h"
#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkImageLinearIteratorWithIndex.h"
#include "itkMetaDataObject.h"

namespace itk
{

template <class TPixel, unsigned int VDimension>
OpenCL1DComplexConjugateToRealImageFilter<TPixel, VDimension>
::OpenCL1DComplexConjugateToRealImageFilter():
  m_PlanComputed(false),
  m_LastImageSize(0),
  m_InputBuffer(0),
  m_OutputBuffer(0)
{
  try
    {
    m_clContext = new cl::Context( CL_DEVICE_TYPE_ALL );
    std::vector< cl::Device > devices = m_clContext->getInfo< CL_CONTEXT_DEVICES > ();
    if( devices.size() < 1 )
      {
      itkExceptionMacro( "No OpenCL devices found." );
      }
    // @todo: code to select the fastest device, or the device that is
    // CL_DEVICE_TYPE_ACCELERATOR
    this->m_clQueue = new cl::CommandQueue( *m_clContext, devices[0] );
    }
  catch( const cl::Error& e )
    {
    itkExceptionMacro( "Error in OpenCL: " << e.what() << "(" << e.err() << ")" ); 
    }
}

template <class TPixel, unsigned int VDimension>
bool
OpenCL1DComplexConjugateToRealImageFilter<TPixel,VDimension>::
Legaldim(int n)
{
  int ifac = 2;
  for(; n % ifac == 0;)
    {
    n /= ifac;
    }
  return (n == 1); // return false if decomposition failed
}

template <typename TPixel, unsigned int Dimension>
void
OpenCL1DComplexConjugateToRealImageFilter<TPixel,Dimension>::
GenerateData()
{
  // get pointers to the input and output
  typename InputImageType::ConstPointer  inputPtr  = this->GetInput();
  typename OutputImageType::Pointer      outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }

  // allocate output buffer memory
  outputPtr->SetBufferedRegion( outputPtr->GetRequestedRegion() );
  outputPtr->Allocate();

  const typename InputImageType::SizeType&   inputSize
    = inputPtr->GetRequestedRegion().GetSize();
  const typename OutputImageType::SizeType&   outputSize
    = outputPtr->GetRequestedRegion().GetSize();

  unsigned int vec_size = inputSize[this->m_Direction];
  if( !this->Legaldim(vec_size) )
    {
    ExceptionObject exception(__FILE__, __LINE__);
    exception.SetDescription("Illegal Array DIM for FFT");
    exception.SetLocation(ITK_LOCATION);
    throw exception;
    }

  cl_int batchSize = 1;
  for( unsigned int i = 0; i < Dimension; i++ )
    {
    batchSize *= outputSize[i];
    }
  unsigned int totalSize = batchSize;
  batchSize /= outputSize[this->m_Direction];
  

  if(this->m_PlanComputed)            // if we've already computed a plan
    {
    // if the image sizes aren't the same,
    // we have to compute the plan again
    if(this->m_LastImageSize != totalSize)
      {
      delete [] this->m_InputBuffer;
      delete [] this->m_OutputBuffer;
      clFFT_DestroyPlan(this->m_Plan);
      this->m_PlanComputed = false;
      }
    }
  if(!this->m_PlanComputed)
    {
    try
      {
      this->m_InputBuffer =
        new OpenCLComplexType[totalSize];
      this->m_OutputBuffer =
        new OpenCLComplexType[totalSize];
      }
    catch( std::bad_alloc & )
      {
      itkExceptionMacro("Problem allocating memory for internal computations");
      }
    this->m_LastImageSize = totalSize;
    clFFT_Dim3 n = { inputSize[this->m_Direction], 1, 1 };
    cl_int error_code;
    this->m_Plan = clFFT_CreatePlan( (*m_clContext)(),
      n,
      clFFT_1D,
      clFFT_InterleavedComplexFormat,
      &error_code );
    if ( ! this->m_Plan || error_code )
      {
      itkExceptionMacro( "Could not create OpenCL FFT Plan." );
      }
    this->m_PlanComputed = true;
    }

  typedef itk::ImageLinearConstIteratorWithIndex< InputImageType >  InputIteratorType;
  typedef itk::ImageLinearIteratorWithIndex< OutputImageType > OutputIteratorType;
  InputIteratorType inputIt( inputPtr, inputPtr->GetRequestedRegion() );
  OutputIteratorType outputIt( outputPtr, outputPtr->GetRequestedRegion() );

  inputIt.SetDirection(this->m_Direction);
  outputIt.SetDirection(this->m_Direction);

  OpenCLComplexType* inputBufferIt = this->m_InputBuffer;
  // for every fft line
  for( inputIt.GoToBegin(); !inputIt.IsAtEnd(); inputIt.NextLine() )
    {
    // copy the input line into our buffer
    inputIt.GoToBeginOfLine();
    while( !inputIt.IsAtEndOfLine() )
      {
      inputBufferIt->real = inputIt.Get().real();
      inputBufferIt->imag = inputIt.Get().imag();
      ++inputIt;
      ++inputBufferIt;
      }
    }

  try
    {
    // do the transform
    cl::Buffer clDataBuffer( *m_clContext,
      CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
      totalSize * sizeof( TPixel ) * 2,
      m_InputBuffer
      );
    cl_command_queue queue = ( *m_clQueue )();
    cl_mem data_in = clDataBuffer();
    cl_mem data_out = clDataBuffer();
    cl_int err = clFFT_ExecuteInterleaved( queue, this->m_Plan, batchSize, clFFT_Inverse, data_in, data_out, 0, NULL, NULL );
    if( err )
      {
      itkExceptionMacro( "Error in clFFT_ExecuteInterleaved(" << err << ")");
      }
    m_clQueue->finish();

    err = m_clQueue->enqueueReadBuffer( clDataBuffer, CL_TRUE, 0, totalSize * sizeof( TPixel ) * 2, m_OutputBuffer );
    }
  catch( const cl::Error& e )
    {
    itkExceptionMacro( "Error in OpenCL: " << e.what() << "(" << e.err() << ")" ); 
    }

  // Follow the convention of the other FFT implementations.
  TPixel normalizationFactor = 1. / 2. ;
  OpenCLComplexType* outputBufferIt = this->m_OutputBuffer;
  // for every fft line
  for( outputIt.GoToBegin(); !outputIt.IsAtEnd(); outputIt.NextLine() )
    {
    // copy the output line into our buffer
    outputIt.GoToBeginOfLine();
    while( !outputIt.IsAtEndOfLine() )
      {
      outputIt.Set( outputBufferIt->real / normalizationFactor );
      ++outputIt;
      ++outputBufferIt;
      }
    }

}

} // namespace itk

#endif //_itkOpenCL1DComplexConjugateToRealImageFilter_txx
