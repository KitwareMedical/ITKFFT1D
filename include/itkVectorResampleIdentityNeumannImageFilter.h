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
#ifndef itkVectorResampleIdentityNeumannImageFilter_h
#define itkVectorResampleIdentityNeumannImageFilter_h

#include "itkImageFunction.h"
#include "itkImageRegionIterator.h"
#include "itkImageToImageFilter.h"
#include "itkVectorInterpolateImageFunction.h"
#include "itkFixedArray.h"
#include "itkSize.h"

namespace itk
{

/** \class VectorResampleIdentityNeumannImageFilter
 * \brief Resample a vector image via an identity transform and use Neumann boundary
 * conditions.
 *
 * Instead of using a default pixel value when a requested pixel value is
 * outside the image boundary, like
 * VectorResampleImageFilter, the filter uses the value at the nearest pixel.
 *
 * Note that the choice of interpolator function can be important.
 * This function is set via SetInterpolator().  The default is
 * itk::VectorLinearInterpolateImageFunction<InputImageType, TInterpolatorPrecisionType>, which
 * is reasonable for ordinary medical images.
 *
 * Since this filter produces an image which is a different size than
 * its input, it needs to override several of the methods defined
 * in ProcessObject in order to properly manage the pipeline execution model.
 * In particular, this filter overrides
 * ProcessObject::GenerateInputRequestedRegion() and
 * ProcessObject::GenerateOutputInformation().
 *
 * This filter is implemented as a multithreaded filter.  It provides a
 * ThreadedGenerateData() method for its implementation.
 *
 * \sa VectorResampleImageFilter
 *
 * \ingroup Ultrasound
 */
template <class TInputImage, class TOutputImage, class TInterpolatorPrecisionType=double>
class ITK_TEMPLATE_EXPORT VectorResampleIdentityNeumannImageFilter:
    public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_ASSIGN(VectorResampleIdentityNeumannImageFilter);

  /** Standard class typedefs. */
  typedef VectorResampleIdentityNeumannImageFilter      Self;
  typedef ImageToImageFilter<TInputImage,TOutputImage>  Superclass;
  typedef SmartPointer<Self>                            Pointer;
  typedef SmartPointer<const Self>                      ConstPointer;

  typedef TInputImage                           InputImageType;
  typedef TOutputImage                          OutputImageType;
  typedef typename InputImageType::Pointer      InputImagePointer;
  typedef typename InputImageType::ConstPointer InputImageConstPointer;
  typedef typename OutputImageType::Pointer     OutputImagePointer;
  typedef typename InputImageType::RegionType   InputImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(VectorResampleIdentityNeumannImageFilter, ImageToImageFilter);

  /** Number of dimensions. */
  itkStaticConstMacro(ImageDimension, unsigned int, TOutputImage::ImageDimension);

  /** Interpolator typedef. */
  typedef VectorInterpolateImageFunction<InputImageType, TInterpolatorPrecisionType> InterpolatorType;
  typedef typename InterpolatorType::Pointer  InterpolatorPointerType;

  /** Image size typedef. */
  typedef Size<itkGetStaticConstMacro(ImageDimension)> SizeType;

  /** Image index typedef. */
  typedef typename TOutputImage::IndexType IndexType;

  /** Image point typedef. */
  typedef typename InterpolatorType::PointType    PointType;

  /** Image pixel value typedef. */
  typedef typename TOutputImage::PixelType   PixelType;
  typedef typename PixelType::ValueType      PixelComponentType;

  /** Typedef to describe the output image region type. */
  typedef typename TOutputImage::RegionType OutputImageRegionType;

  /** Image spacing,origin and direction typedef */
  typedef typename TOutputImage::SpacingType   SpacingType;
  typedef typename TOutputImage::PointType     OriginPointType;
  typedef typename TOutputImage::DirectionType DirectionType;

  /** Set the interpolator function.  The default is
   * itk::VectorLinearInterpolateImageFunction<InputImageType, TInterpolatorPrecisionType>.  */
  itkSetObjectMacro( Interpolator, InterpolatorType );

  /** Get a pointer to the interpolator function. */
  itkGetConstObjectMacro( Interpolator, InterpolatorType );

  /** Set the size of the output image. */
  itkSetMacro( Size, SizeType );

  /** Get the size of the output image. */
  itkGetConstReferenceMacro( Size, SizeType );

  /** Set the output image spacing. */
  itkSetMacro(OutputSpacing, SpacingType);
  virtual void SetOutputSpacing( const double* values);

  /** Get the output image spacing. */
  itkGetConstReferenceMacro( OutputSpacing, SpacingType );

  /** Set the output image origin. */
  itkSetMacro(OutputOrigin, PointType);
  virtual void SetOutputOrigin( const double* values);

  /** Get the output image origin. */
  itkGetConstReferenceMacro( OutputOrigin, PointType );

  /** Set the output direciton cosine matrix. */
  itkSetMacro(OutputDirection, DirectionType);
  itkGetConstReferenceMacro(OutputDirection, DirectionType);

  /** Set the start index of the output largest possible region.
   * The default is an index of all zeros. */
  itkSetMacro( OutputStartIndex, IndexType );

  /** Get the start index of the output largest possible region. */
  itkGetConstReferenceMacro( OutputStartIndex, IndexType );


#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(InputHasNumericTraitsCheck,
    (Concept::HasNumericTraits<typename TInputImage::PixelType::ValueType>));
  itkConceptMacro(OutputHasNumericTraitsCheck,
    (Concept::HasNumericTraits<PixelComponentType>));
  /** End concept checking */
#endif

protected:
  VectorResampleIdentityNeumannImageFilter();
  ~VectorResampleIdentityNeumannImageFilter() {};
  void PrintSelf(std::ostream& os, Indent indent) const override;

  /** VectorResampleIdentityNeumannImageFilter can be implemented as a multithreaded filter.  Therefore,
   * this implementation provides a ThreadedGenerateData() routine which
   * is called for each processing thread. The output image data is allocated
   * automatically by the superclass prior to calling ThreadedGenerateData().
   * ThreadedGenerateData can only write to the portion of the output image
   * specified by the parameter "outputRegionForThread"
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() */
  void DynamicThreadedGenerateData(const OutputImageRegionType& outputRegionForThread ) override;

  /** VectorResampleIdentityNeumannImageFilter produces an image which is a different size
   * than its input.  As such, it needs to provide an implementation
   * for GenerateOutputInformation() in order to inform the pipeline
   * execution model.  The original documentation of this method is
   * below. \sa ProcessObject::GenerateOutputInformaton() */
  void GenerateOutputInformation() override;

  /** VectorResampleIdentityNeumannImageFilter needs a different input requested region than
   * the output requested region.  As such, VectorResampleIdentityNeumannImageFilter needs
   * to provide an implementation for GenerateInputRequestedRegion()
   * in order to inform the pipeline execution model.
   * \sa ProcessObject::GenerateInputRequestedRegion() */
  void GenerateInputRequestedRegion() override;

  /** This method is used to set the state of the filter before
   * multi-threading. */
  void BeforeThreadedGenerateData() override;

  /** This method is used to set the state of the filter after
   * multi-threading. */
  void AfterThreadedGenerateData() override;

  /** Method Compute the Modified Time based on changed to the components. */
  ModifiedTimeType GetMTime() const override;

private:
  SizeType                m_Size;       // Size of the output image
  InterpolatorPointerType m_Interpolator;
  // Image function for interpolation
  SpacingType             m_OutputSpacing; // output image spacing
  PointType               m_OutputOrigin;  // output image origin
  DirectionType           m_OutputDirection; // output image direction cosines
  IndexType               m_OutputStartIndex; // output start index
};


} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkVectorResampleIdentityNeumannImageFilter.hxx"
#endif

#endif
