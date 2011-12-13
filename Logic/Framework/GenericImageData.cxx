/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.14 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
// ITK Includes
#include "itkOrientedImage.h"
#include "itkImageIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkUnaryFunctorImageFilter.h"
#include "UnaryFunctorCache.h"
#include "itkRGBAPixel.h"
#include "IRISSlicer.h"
#include "IRISApplication.h"
#include <list>
#include "ImageWrapperFactory.h"

/** Borland compiler is very lazy so we need to instantiate the template
 *  by hand */
#if defined(__BORLANDC__)
typedef IRISSlicer<unsigned char> GenericImageDataDummyIRISSlicerTypeUchar;
typedef itk::SmartPointer<GenericImageDataDummyIRISSlicerTypeUchar> GenericImageDataDummySmartPointerSlicerType;
typedef IRISSlicer<short> GenericImageDataDummyIRISSlicerTypeShort;
typedef itk::SmartPointer<GenericImageDataDummyIRISSlicerTypeShort> GenericImageDataDummySmartPointerSlicerShortType;
typedef itk::ImageRegion<3> GenericImageDataBorlandDummyImageRegionType;
typedef itk::ImageRegion<2> GenericImageDataBorlandDummyImageRegionType2;
typedef itk::ImageBase<3> GenericImageDataBorlandDummyImageBaseType;
typedef itk::ImageBase<2> GenericImageDataBorlandDummyImageBaseType2;
typedef itk::Image<unsigned char,3> GenericImageDataBorlandDummyImageType;
typedef itk::Image<unsigned char,2> GenericImageDataBorlandDummyImageType2;
typedef itk::ImageRegionConstIterator<GenericImageDataBorlandDummyImageType> GenericImageDataBorlandDummyConstIteratorType;
typedef itk::Image<short,3> GenericImageDataBorlandDummyShortImageType;
typedef itk::Image<short,2> GenericImageDataBorlandDummyShortImageType2;
typedef itk::Image<itk::RGBAPixel<unsigned char>,2> GenericImageDataBorlandDummyShortImageTypeRGBA;
typedef itk::ImageRegionConstIterator<GenericImageDataBorlandDummyShortImageType> GenericImageDataBorlandDummyConstIteratorShortType;
typedef itk::MinimumMaximumImageCalculator<GenericImageDataBorlandDummyShortImageType> GenericImageDataBorlandDummyMinMaxCalc;
#endif

#include "GreyImageWrapper.h"
#if defined(__BORLANDC__)
typedef CachingUnaryFunctor<short,unsigned char,GreyImageWrapper::IntensityFunctor> GenericImageDataBorlamdCachingUnaryFunctor;
typedef itk::UnaryFunctorImageFilter<GenericImageDataBorlandDummyShortImageType,GenericImageDataBorlandDummyImageType2,GenericImageDataBorlamdCachingUnaryFunctor> GenericImageDataDummyFunctorType;
typedef itk::SmartPointer<GenericImageDataDummyFunctorType> GenericImageDataDummyFunctorTypePointerType;
#endif

#include "GenericImageData.h"

#include "LabelImageWrapper.h"

#include "RGBImageWrapper.h"

// System includes
#include <fstream>
#include <iostream>
#include <iomanip>


void 
GenericImageData
::SetSegmentationVoxel(const Vector3ui &index, LabelType value)
{
  // Make sure that the main image data and the segmentation data exist
  assert(IsSegmentationLoaded());

  // Store the voxel
  m_LabelWrapper->GetVoxelForUpdate(index) = value;

  // Mark the image as modified
  m_LabelWrapper->GetImage()->Modified();
}

GenericImageData
::GenericImageData(IRISApplication *parent) 
{
  // Copy the parent object
  m_Parent = parent;

  // Make main image wrapper point to grey wrapper initially
  m_MainImageWrapper = NULL;
  m_GreyImageWrapper = NULL;
  m_RGBImageWrapper = NULL;

  // Pass the label table from the parent to the label wrapper
  m_LabelWrapper = NULL;
  
  // Add to the relevant lists
  m_Wrappers[LayerIterator::MAIN_ROLE].push_back(m_MainImageWrapper);
  m_Wrappers[LayerIterator::LABEL_ROLE].push_back(m_MainImageWrapper);
}

GenericImageData
::~GenericImageData()
{
  UnloadMainImage();
}

Vector3d 
GenericImageData
::GetImageSpacing() 
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetImageBase()->GetSpacing().GetVnlVector();
}

Vector3d 
GenericImageData
::GetImageOrigin() 
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetImageBase()->GetOrigin().GetVnlVector();
}

void 
GenericImageData
::SetGreyImage(GreyImageType *image,
               const ImageCoordinateGeometry &newGeometry,
               const InternalToNativeFunctor &native)
{
  // Clean up wrappers
  delete m_MainImageWrapper;

  // No RGB
  m_RGBImageWrapper = NULL;

  // Create a main wrapper of fixed type.
  m_MainImageWrapper = m_GreyImageWrapper = new GreyImageWrapper<GreyType>();

  // Set properties
  m_GreyImageWrapper->SetImage(image);
  m_GreyImageWrapper->SetNativeMapping(native);
  m_GreyImageWrapper->UpdateIntensityMapFunction();

  // Is this redundant?
  SetMainImageCommon(newGeometry);
}

void
GenericImageData
::SetRGBImage(RGBImageType *image,
              const ImageCoordinateGeometry &newGeometry) 
{
  // Clean up wrappers
  delete m_MainImageWrapper;

  // No RGB
  m_GreyImageWrapper = NULL;

  // Create a main wrapper through factory
  m_MainImageWrapper = m_RGBImageWrapper = new RGBImageWrapper<unsigned char>();
  m_RGBImageWrapper->SetImage(image);

  SetMainImageCommon(newGeometry);
}

void
GenericImageData
::SetMainImageCommon(const ImageCoordinateGeometry &newGeometry)
{
  // Make the wrapper the main image
  m_Wrappers[LayerIterator::MAIN_ROLE][0] = m_MainImageWrapper;

  // Initialize the segmentation data to zeros
  delete m_LabelWrapper;
  m_LabelWrapper = new LabelImageWrapper();
  m_LabelWrapper->InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);
  m_LabelWrapper->SetLabelColorTable(m_Parent->GetColorLabelTable());

  m_Wrappers[LayerIterator::LABEL_ROLE][0] = m_LabelWrapper;

  // Set opaque
  m_MainImageWrapper->SetAlpha(255);

  // Pass the coordinate transform to the wrappers
  SetImageGeometry(newGeometry);
}

void
GenericImageData
::UnloadMainImage()
{
  // First unload the overlays if exist
  UnloadOverlays();

  // Clear the main image wrappers
  delete m_MainImageWrapper;
  m_MainImageWrapper = NULL;
  m_GreyImageWrapper = NULL;
  m_RGBImageWrapper = NULL;

  // Reset the label wrapper
  delete m_LabelWrapper;
  m_LabelWrapper = NULL;

  m_Wrappers[LayerIterator::MAIN_ROLE][0] = NULL;
  m_Wrappers[LayerIterator::LABEL_ROLE][0] = NULL;
}

void
GenericImageData
::AddGreyOverlay(GreyImageType *image,
                 const InternalToNativeFunctor &native)
{
  // Check that the image matches the size of the main image
  assert(m_MainImageWrapper->GetBufferedRegion() ==
         image->GetBufferedRegion());

  // Pass the image to a Grey image wrapper
  GreyImageWrapper<GreyType> *wrapper = new GreyImageWrapper<GreyType>();
  wrapper->SetImage(image);
  wrapper->SetNativeMapping(native);
  wrapper->UpdateIntensityMapFunction();
  wrapper->SetAlpha(128);

  AddOverlayCommon(wrapper);
}

void
GenericImageData
::AddRGBOverlay(RGBImageType  *image)
{
  // Check that the image matches the size of the main image
  assert(m_MainImageWrapper->GetBufferedRegion() ==
         image->GetBufferedRegion());

  // Pass the image to a RGB image wrapper
  RGBImageWrapper<unsigned char> *wrapper
      = new RGBImageWrapper<unsigned char>();
  wrapper->SetImage(image);

  AddOverlayCommon(wrapper);
}

void
GenericImageData
::AddOverlayCommon(ImageWrapperBase *overlay)
{
  // Set up the alpha
  overlay->SetAlpha(128);

  // Sync up spacing between the main and overlay image
  overlay->GetImageBase()->SetSpacing(
        m_MainImageWrapper->GetImageBase()->GetSpacing());

  overlay->GetImageBase()->SetOrigin(
        m_MainImageWrapper->GetImageBase()->GetOrigin());

  overlay->GetImageBase()->SetDirection(
        m_MainImageWrapper->GetImageBase()->GetDirection());

  // Propagate the geometry information to this wrapper
  for(unsigned int iSlice = 0; iSlice < 3; iSlice ++)
    {
    overlay->SetImageToDisplayTransform(
      iSlice, m_ImageGeometry.GetImageToDisplayTransform(iSlice));
    }

  // Add to the overlay wrapper list
  m_Wrappers[LayerIterator::OVERLAY_ROLE].push_back(overlay);
}

void
GenericImageData
::UnloadOverlays()
{
  while (m_Wrappers[LayerIterator::OVERLAY_ROLE].size() > 0)
    UnloadOverlayLast();
}

void
GenericImageData
::UnloadOverlayLast()
{
  // Make sure at least one grey overlay is loaded
  if (!IsOverlayLoaded())
    return;

  // Release the data associated with the last overlay
  ImageWrapperBase *wrapper = m_Wrappers[LayerIterator::OVERLAY_ROLE].back();
  delete wrapper;
  wrapper = NULL;

  // Clear it off the wrapper lists
  m_Wrappers[LayerIterator::OVERLAY_ROLE].pop_back();
}

void
GenericImageData
::SetSegmentationImage(LabelImageType *newLabelImage) 
{
  // Check that the image matches the size of the grey image
  assert(m_MainImageWrapper->IsInitialized() &&
    m_MainImageWrapper->GetBufferedRegion() == 
         newLabelImage->GetBufferedRegion());

  // Pass the image to the segmentation wrapper (why this and not create a
  // new label wrapper? Why should a wrapper have longer lifetime than an
  // image that it wraps around
  m_LabelWrapper->SetImage(newLabelImage);

  // Sync up spacing between the main and label image
  newLabelImage->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
  newLabelImage->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());
}

bool
GenericImageData
::IsGreyLoaded()
{
  return m_GreyImageWrapper != NULL;
}

bool
GenericImageData
::IsOverlayLoaded()
{
  return (m_Wrappers[LayerIterator::OVERLAY_ROLE].size() > 0);
}

bool
GenericImageData
::IsRGBLoaded()
{
  return m_RGBImageWrapper != NULL;
}

bool
GenericImageData
::IsSegmentationLoaded()
{
  return m_LabelWrapper && m_LabelWrapper->IsInitialized();
}

void
GenericImageData
::SetCrosshairs(const Vector3ui &crosshairs)
{
  // Set crosshairs in all wrappers
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer() && lit.GetLayer()->IsInitialized())
      lit.GetLayer()->SetSliceIndex(crosshairs);
}

GenericImageData::RegionType
GenericImageData
::GetImageRegion() const
{
  assert(m_MainImageWrapper->IsInitialized());
  return m_MainImageWrapper->GetBufferedRegion();
}

void
GenericImageData
::SetImageGeometry(const ImageCoordinateGeometry &geometry)
{
  m_ImageGeometry = geometry;
  for(LayerIterator lit(this); !lit.IsAtEnd(); ++lit)
    if(lit.GetLayer() && lit.GetLayer()->IsInitialized())
      {
      // Set the direction matrix in the image
      lit.GetLayer()->GetImageBase()->SetDirection(
        itk::Matrix<double,3,3>(geometry.GetImageDirectionCosineMatrix()));

      // Update the geometry for each slice
      for(unsigned int iSlice = 0;iSlice < 3;iSlice ++)
        {
        lit.GetLayer()->SetImageToDisplayTransform(
          iSlice,m_ImageGeometry.GetImageToDisplayTransform(iSlice));
        }
      }
}

unsigned int GenericImageData::GetNumberOfLayers(int role_filter)
{
  unsigned int n = 0;
  for(int role = LayerIterator::MAIN_ROLE;
      role != LayerIterator::NO_ROLE; role = role << 1)
    {
    if(role_filter | role)
      {
      WrapperStorage::iterator it = m_Wrappers.find((LayerRole) role);
      if(it != m_Wrappers.end())
        n += it->second.size();
      }
    }

  return n;
}

/*
inline
ImageWrapperBase *
GenericImageData
::GetLayer(unsigned int layer) const
{
  return layer == 0 ? m_MainImageWrapper : m_OverlayWrappers[layer-1];
}

inline
GreyImageWrapperBase *
GenericImageData
::GetLayerAsGray(unsigned int layer) const
{
  return dynamic_cast<GreyImageWrapperBase *>(GetLayer(layer));
}

inline
RGBImageWrapperBase *
GenericImageData
::GetLayerAsRGB(unsigned int layer) const
{
  return dynamic_cast<RGBImageWrapperBase *>(GetLayer(layer));
}

*/



LayerIterator
::LayerIterator(
    GenericImageData *data, int role_filter)
{
  m_ImageData = data;
  m_Wrappers = &data->m_Wrappers;
  m_RoleFilter = role_filter;

  // Find the first role that fits the filter and is not empty
  m_IterRole = MAIN_ROLE;
  FindNextUsableRole();
}

void LayerIterator::FindNextUsableRole()
{
  for(; m_IterRole != NO_ROLE; m_IterRole = (LayerRole) (m_IterRole << 1))
    {
    if(m_RoleFilter | m_IterRole)
      {
      WrapperStorage::iterator it = m_Wrappers->find(m_IterRole);
      if(it != m_Wrappers->end() && it->second.size())
        {
        m_Iter = it->second.begin();
        break;
        }
      }
    }
}

bool LayerIterator
::IsAtEnd()
{
  // We are at end when there are no roles left
  return (m_IterRole == NO_ROLE);
}

LayerIterator &
LayerIterator::operator ++()
{
  // We should never be pointing to the end
  m_Iter++;
  if(m_Iter == (*m_Wrappers)[m_IterRole].end())
    {
    m_IterRole = (LayerRole)(m_IterRole << 1);
    FindNextUsableRole();
    }
  return *this;
}

ImageWrapperBase * LayerIterator::GetLayer()
{
  assert(m_IterRole < NO_ROLE);
  assert(m_Wrappers->find(m_IterRole) != m_Wrappers->end());
  assert(m_Iter != (*m_Wrappers)[m_IterRole].end());
  return *m_Iter;
}

GreyImageWrapperBase * LayerIterator::GetLayerAsGray()
{
  return dynamic_cast<GreyImageWrapperBase *>(this->GetLayer());
}

RGBImageWrapperBase * LayerIterator::GetLayerAsRGB()
{
  return dynamic_cast<RGBImageWrapperBase *>(this->GetLayer());
}

LayerIterator::LayerRole
LayerIterator::GetRole()
{
  return m_IterRole;
}




