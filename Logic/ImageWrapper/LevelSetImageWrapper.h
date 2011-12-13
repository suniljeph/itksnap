/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LevelSetImageWrapper.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 21:38:16 $
  Version:   $Revision: 1.4 $
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
#ifndef __LevelSetImageWrapper_h_
#define __LevelSetImageWrapper_h_

#include "ScalarImageWrapper.h"

// Forward references to ITK
namespace itk {
  template <class TInput,class TOutput,class TFunctor> 
    class UnaryFunctorImageFilter;
};
class ColorLabel;

/**
 * \class LevelSetImageWrapper
 * \brief Image wraper for level set images in SNAP
 *
 * The slices generated by this wrapper are processed such that
 * the interior (negative) regions of the level set image are mapped
 * to an RGB value and the exterior regions are black.
 * 
 * \sa ImageWrapper
 */
class LevelSetImageWrapper : public ScalarImageWrapper<float>
{
public:

  /** Set the color label for inside */
  void SetColorLabel(const ColorLabel &label);
  
  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);

  /** Constructor initializes mappers */
  LevelSetImageWrapper();

  /** Destructor */
  ~LevelSetImageWrapper();

private:
  /**
   * A very simple functor used to map intensities
   */
  class MappingFunctor 
  {
  public:
    DisplayPixelType operator()(float in);
    DisplayPixelType m_InsidePixel;
    DisplayPixelType m_OutsidePixel;
    
    // Dummy equality operators, since there is no data here
    bool operator == (const MappingFunctor &z) const 
      { 
      return
        m_InsidePixel == z.m_InsidePixel &&
        m_OutsidePixel == z.m_OutsidePixel;
      }
      
    bool operator != (const MappingFunctor &z) const 
      { return !(*this == z); }
  };  
  
  // Type of the display intensity mapping filter used when the 
  // input is a in-out image
  typedef itk::UnaryFunctorImageFilter<
    ImageWrapper<float>::SliceType,DisplaySliceType,MappingFunctor> 
    IntensityFilterType;
  typedef itk::SmartPointer<IntensityFilterType> IntensityFilterPointer;

  /** 
   * The filters used to remap internal level set image 
   * to a color display image
   */
  IntensityFilterPointer m_DisplayFilter[3];

  /** The currently used overlay functor */
  MappingFunctor m_MappingFunctor;
};

#endif // __LevelSetImageWrapper_h_
