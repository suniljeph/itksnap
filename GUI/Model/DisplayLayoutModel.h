#ifndef DISPLAYLAYOUTMODEL_H
#define DISPLAYLAYOUTMODEL_H

class GlobalUIModel;

#include "PropertyModel.h"

/**
 * @brief A model that manages display layout properties. This model is
 * just a collection of specific properties
 */
class DisplayLayoutModel : public AbstractModel
{
public:

  // ITK macros
  irisITKObjectMacro(DisplayLayoutModel, AbstractModel)

  // Parent event type for all events fired by this model
  itkEventMacro(DisplayLayoutChangeEvent, IRISEvent)

  // Event fired when the layout of the slice panels changes
  itkEventMacro(ViewPanelLayoutChangeEvent, DisplayLayoutChangeEvent)

  // Event fired when the layout of the overlays in a slice panel changes
  itkEventMacro(OverlayLayoutChangeEvent, DisplayLayoutChangeEvent)

  // This model fires a ModelUpdateEvent whenever there is a change in the
  // display layout
  FIRES(ViewPanelLayoutChangeEvent)
  FIRES(OverlayLayoutChangeEvent)

  /** Layout of the SNAP slice views */
  enum ViewPanelLayout {
    FOUR_VIEWS=0, AXIAL_ONLY, CORONAL_ONLY, SAGITTAL_ONLY, THREED_ONLY
  };

  typedef AbstractPropertyModel<ViewPanelLayout> AbstractViewPanelLayoutProperty;

  /** Parent model */
  irisGetSetMacro(ParentModel, GlobalUIModel *)

  /** Model managing the view panel layouts */
  irisGetMacro(ViewPanelLayoutModel, AbstractViewPanelLayoutProperty *)

  /**
   * A model handing the layout of the layers in a slice view. This gives
   * the number of rows and columns into which the slice views are broken
   * up when displaying overlays. When this is 1x1, the overlays are painted
   * on top of each other using transparency. Otherwise, each overlay is shown
   * in its own cell.
   */
  irisGetMacro(SliceViewCellLayoutModel, AbstractSimpleUIntVec2Property *)

protected:

  GlobalUIModel *m_ParentModel;

  typedef ConcretePropertyModel<ViewPanelLayout> ConcreteViewPanelLayoutProperty;
  SmartPtr<ConcreteViewPanelLayoutProperty> m_ViewPanelLayoutModel;

  SmartPtr<ConcreteSimpleUIntVec2Property> m_SliceViewCellLayoutModel;

  DisplayLayoutModel();
  virtual ~DisplayLayoutModel() {}
};

#endif // DISPLAYLAYOUTMODEL_H