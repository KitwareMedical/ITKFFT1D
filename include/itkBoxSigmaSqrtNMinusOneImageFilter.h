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
#ifndef itkBoxSigmaSqrtNMinusOneImageFilter_h
#define itkBoxSigmaSqrtNMinusOneImageFilter_h

#include "itkBoxUtilities.h"
#include "itkBoxImageFilter.h"
#include "itkVersion.h"

namespace itk
{

// copied and tweaked from BoxSigmaCalculatorFunction
template <class TInputImage, class TOutputImage>
void
BoxSigmaSqrtNMinusOneCalculatorFunction(const TInputImage * accImage,
                          TOutputImage * outputImage,
                          const typename TInputImage::RegionType & inputRegion,
                          const typename TOutputImage::RegionType & outputRegion,
                          const typename TInputImage::SizeType & Radius)
{
  // typedefs
  typedef TInputImage                                         InputImageType;
  typedef typename TInputImage::RegionType                    RegionType;
  typedef typename TInputImage::SizeType                      SizeType;
  typedef typename TInputImage::IndexType                     IndexType;
  typedef typename TInputImage::OffsetType                    OffsetType;
  typedef typename TInputImage::OffsetValueType               OffsetValueType;
  typedef TOutputImage                                        OutputImageType;
  typedef typename TOutputImage::PixelType                    OutputPixelType;
  typedef typename TInputImage::PixelType                     InputPixelType;
   // use the face generator for speed
  typedef typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>
                                                              FaceCalculatorType;
  typedef typename FaceCalculatorType::FaceListType           FaceListType;
  typedef typename FaceCalculatorType::FaceListType::iterator FaceListTypeIt;
  FaceCalculatorType faceCalculator;

  FaceListType faceList;
  FaceListTypeIt fit;
  ZeroFluxNeumannBoundaryCondition<TInputImage> nbc;

  // this process is actually slightly asymmetric because we need to
  // subtract rectangles that are next to our kernel, not overlapping it
  SizeType kernelSize;
  SizeType internalRadius;
  SizeType RegionLimit;
  IndexType RegionStart = inputRegion.GetIndex();
  for( unsigned int i = 0; i < TInputImage::ImageDimension; ++i )
    {
    kernelSize[i] = Radius[i] * 2 + 1;
    internalRadius[i] = Radius[i] + 1;
    RegionLimit[i] = inputRegion.GetSize()[i] + RegionStart[i] - 1;
    }

  typedef typename NumericTraits<OutputPixelType>::RealType AccPixType;
  // get a set of offsets to corners for a unit hypercube in this image
  std::vector<OffsetType> UnitCorners = CornerOffsets<TInputImage>(accImage);
  std::vector<OffsetType> RealCorners;
  std::vector<AccPixType> Weights;
  // now compute the weights
  for (unsigned int k = 0; k < UnitCorners.size(); ++k)
    {
    int prod = 1;
    OffsetType ThisCorner;
    for (unsigned int i = 0; i < TInputImage::ImageDimension; ++i)
      {
      prod *= UnitCorners[k][i];
      if (UnitCorners[k][i] > 0)
        {
        ThisCorner[i] = Radius[i];
        }
      else
        {
        ThisCorner[i] = -( (int) Radius[i]+1);
        }
      }
    Weights.push_back((AccPixType)prod);
    RealCorners.push_back(ThisCorner);
    }


  faceList = faceCalculator(accImage, outputRegion, internalRadius);
  // start with the body region
  for (fit = faceList.begin(); fit != faceList.end(); ++fit)
    {
    if (fit == faceList.begin())
      {
      // this is the body region. This is meant to be an optimized
      // version that doesn't use neigborhood regions
      // compute the various offsets
      AccPixType pixelscount = 1;
      for (unsigned int i = 0; i < TInputImage::ImageDimension; ++i)
        {
        pixelscount *= (AccPixType)(2*Radius[i] + 1);
        }

      typedef ImageRegionIterator<OutputImageType>     OutputIteratorType;
      typedef ImageRegionConstIterator<InputImageType> InputIteratorType;

      typedef std::vector<InputIteratorType> CornerItVecType;
      CornerItVecType CornerItVec;
      // set up the iterators for each corner
      for (unsigned int k = 0; k < RealCorners.size(); ++k)
        {
        typename InputImageType::RegionType tReg=(*fit);
        tReg.SetIndex(tReg.GetIndex() + RealCorners[k]);
        InputIteratorType tempIt(accImage, tReg);
        tempIt.GoToBegin();
        CornerItVec.push_back(tempIt);
        }
      // set up the output iterator
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        AccPixType Sum = 0;
        AccPixType SquareSum = 0;
        // check each corner
        for (unsigned int k = 0; k < CornerItVec.size(); ++k)
          {
          const InputPixelType & i = CornerItVec[k].Get();
          Sum += Weights[k] * i[0];
          SquareSum += Weights[k] * i[1];
          // increment each corner iterator
          ++(CornerItVec[k]);
          }

        oIt.Set(static_cast<OutputPixelType>( std::sqrt(  SquareSum - Sum*Sum/pixelscount ) ) );
        }
      }
    else
      {
      // now we need to deal with the border regions
      typedef ImageRegionIteratorWithIndex<OutputImageType> OutputIteratorType;
      OutputIteratorType oIt(outputImage, *fit);
      // now do the work
      for (oIt.GoToBegin(); !oIt.IsAtEnd(); ++oIt)
        {
        // figure out the number of pixels in the box by creating an
        // equivalent region and cropping - this could probably be
        // included in the loop below.
        RegionType currentKernelRegion;
        currentKernelRegion.SetSize( kernelSize );
        // compute the region's index
        IndexType kernelRegionIdx = oIt.GetIndex();
        IndexType CentIndex = kernelRegionIdx;
        for( unsigned int i = 0; i < TInputImage::ImageDimension; ++i )
          {
          kernelRegionIdx[i] -= Radius[i];
          }
        currentKernelRegion.SetIndex( kernelRegionIdx );
        currentKernelRegion.Crop( inputRegion );
        long edgepixelscount = currentKernelRegion.GetNumberOfPixels();
        AccPixType Sum = 0;
        AccPixType SquareSum = 0;
        // rules are : for each corner,
        //               for each dimension
        //                  if dimension offset is positive -> this is
        //                  a leading edge. Crop if outside the input
        //                  region
        //                  if dimension offset is negative -> this is
        //                  a trailing edge. Ignore if it is outside
        //                  image region
        for (unsigned int k = 0; k < RealCorners.size(); ++k)
          {
          IndexType ThisCorner = CentIndex + RealCorners[k];
          bool IncludeCorner = true;
          for (unsigned int j = 0; j < TInputImage::ImageDimension; ++j)
            {
            if (UnitCorners[k][j] > 0)
              {
              // leading edge - crop it
              if( ThisCorner[j] > static_cast< OffsetValueType >( RegionLimit[j]) )
                {
                ThisCorner[j] = static_cast< OffsetValueType>( RegionLimit[j] );
                }
              }
            else
              {
              // trailing edge - check bounds
              if (ThisCorner[j] < RegionStart[j])
                {
                IncludeCorner = false;
                break;
                }
              }
            }
          if (IncludeCorner)
            {
            const InputPixelType & i = accImage->GetPixel(ThisCorner);
            Sum += Weights[k] * i[0];
            SquareSum += Weights[k] * i[1];
            }
          }

        oIt.Set(static_cast<OutputPixelType>( std::sqrt( SquareSum - Sum*Sum/edgepixelscount ) ) );
        }
      }
    }
}


/**
 * \class BoxSigmaSqrtNMinusOneImageFilter
 * \brief Similar to the BoxSigmaImageFilter, that calculates the standard
 * deviation over a box, but calculates the standard deviation time sqrt( N-1 ).
 * Used in calculating the normalized cross correlation.
 *
 * \sa BoxSigmaImageFilter
 * \sa NormalizedCrossCorrelationMetricImageFilter
 *
 * \ingroup Ultrasound
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_TEMPLATE_EXPORT BoxSigmaSqrtNMinusOneImageFilter :
    public BoxImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef BoxSigmaSqrtNMinusOneImageFilter           Self;
  typedef BoxImageFilter<TInputImage, TOutputImage>  Superclass;
  typedef SmartPointer<Self>                         Pointer;
  typedef SmartPointer<const Self>                   ConstPointer;

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(BoxSigmaSqrtNMinusOneImageFilter, BoxImageFilter);

  /** Image related typedefs. */
  typedef TInputImage                                InputImageType;
  typedef TOutputImage                               OutputImageType;
  typedef typename TInputImage::RegionType           RegionType;
  typedef typename TInputImage::SizeType             SizeType;
  typedef typename TInputImage::IndexType            IndexType;
  typedef typename TInputImage::PixelType            PixelType;
  typedef typename TInputImage::OffsetType           OffsetType;
  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;
  typedef typename TOutputImage::PixelType           OutputPixelType;

  /** Image related typedefs. */
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);


#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimension,
                  (Concept::SameDimension<itkGetStaticConstMacro(InputImageDimension),itkGetStaticConstMacro(OutputImageDimension)>));


  /** End concept checking */
#endif


protected:
  BoxSigmaSqrtNMinusOneImageFilter();
  ~BoxSigmaSqrtNMinusOneImageFilter() {};

  /** Multi-thread version GenerateData. */
#if ITK_VERSION_MAJOR < 5
  void ThreadedGenerateData (const OutputImageRegionType&
                              outputRegionForThread,
                              ThreadIdType threadId) override;
#else
  void DynamicThreadedGenerateData (const OutputImageRegionType& outputRegionForThread) override;
#endif

private:
  BoxSigmaSqrtNMinusOneImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

}; // end of class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBoxSigmaSqrtNMinusOneImageFilter.hxx"
#endif

#endif
