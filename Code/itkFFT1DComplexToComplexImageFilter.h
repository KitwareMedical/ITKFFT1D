#ifndef __itkFFT1DComplexToComplexImageFilter_h
#define __itkFFT1DComplexToComplexImageFilter_h

#include <complex>

#include "itkImage.h"
#include "itkImageToImageFilter.h"
#include "itkImageRegionSplitterDirection.h"

namespace itk
{
/** @class itkFFT1DComplexToComplexImageFilter
 * @brief Perform the Fast Fourier Transform, complex input to complex output,
 * but only along one dimension.
 *
 * The direction of the transform, 'Forward' or 'Inverse', can be set with
 * SetTransformDirection() and GetTransformDirection().
 *
 * The dimension along which to apply to filter can be specified with
 * SetDirection() and GetDirection().
 *
 * @ingroup FourierTransform
 */
template <class TPixel, unsigned int VDimension = 3>
class ITK_EXPORT FFT1DComplexToComplexImageFilter:
  public ImageToImageFilter< Image< std::complex< TPixel >, VDimension >,
			     Image< std::complex< TPixel >, VDimension > >
{
public:
  /** Standard class typedefs. */ 
  typedef Image< std::complex< TPixel > , VDimension> InputImageType;
  typedef Image< std::complex< TPixel > , VDimension> OutputImageType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;

  typedef FFT1DComplexToComplexImageFilter		  Self;
  typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef SmartPointer<Self>                                      Pointer;
  typedef SmartPointer<const Self>                                ConstPointer;

  itkStaticConstMacro(ImageDimension, unsigned int,
                      InputImageType::ImageDimension );

  itkTypeMacro( FFT1DComplexToComplexImageFilter, ImageToImageFilter );

  /** Customized object creation methods that support configuration-based 
    * selection of FFT implementation.
    *
    * Default implementation is VnlFFT1D.
    */
  static Pointer New(void);

  /** Transform direction. */
  typedef enum { DIRECT = 1, INVERSE } TransformDirectionType;

  /** Set/Get the direction in which the transform will be applied.
   * By selecting DIRECT, this filter will perform a direct (forward) Fourier
   * Transform.
   * By selecting INVERSE, this filter will perform an inverse Fourier
   * Transform. */
  itkSetMacro( TransformDirection, TransformDirectionType );
  itkGetConstMacro( TransformDirection, TransformDirectionType );

  /** Get the direction in which the filter is to be applied. */
  itkGetMacro(Direction, unsigned int);

  /** Set the direction in which the filter is to be applied. */
  itkSetMacro(Direction, unsigned int);

protected:
  FFT1DComplexToComplexImageFilter();
  virtual ~FFT1DComplexToComplexImageFilter() {}
  void PrintSelf(std::ostream& os, Indent indent) const;

  virtual void GenerateInputRequestedRegion();
  virtual void EnlargeOutputRequestedRegion(DataObject *output);

  /** Direction in which the filter is to be applied
   * this should be in the range [0,ImageDimension-1]. */
  unsigned int m_Direction;

  /** Direction to apply the transform (forward/inverse). */
  TransformDirectionType m_TransformDirection;

  virtual void BeforeThreadedGenerateData();

  /** Override to return a splitter that does not split along the direction we
   * are performing the transform. */
  virtual const ImageRegionSplitterBase* GetImageRegionSplitter(void) const;

private:
  FFT1DComplexToComplexImageFilter( const Self& );
  void operator=( const Self& );

  ImageRegionSplitterDirection::Pointer m_ImageRegionSplitter;
};
}

#ifndef ITK_MANUAL_INSTANTIATION
#ifndef __itkVnlFFT1DComplexToComplexImageFilter_h
#ifndef __itkVnlFFT1DComplexToComplexImageFilter_txx
#ifndef __itkFFTW1DComplexToComplexImageFilter_h
#ifndef __itkFFTW1DComplexToComplexImageFilter_txx
#ifndef __itkOpenCL1DComplexToComplexImageFilter_h
#ifndef __itkOpenCL1DComplexToComplexImageFilter_txx
#include "itkFFT1DComplexToComplexImageFilter.txx"
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#endif // __itkFFT1DComplexToComplexImageFilter_h
