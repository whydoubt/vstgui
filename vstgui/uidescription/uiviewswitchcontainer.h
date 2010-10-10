//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework not only for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2010, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __uiviewswitchcontainer__
#define __uiviewswitchcontainer__

#include "../lib/cviewcontainer.h"
#include "uiviewfactory.h"
#include <vector>

namespace VSTGUI {
class IViewSwitchController;

//-----------------------------------------------------------------------------
class UIViewSwitchContainer : public CViewContainer
{
public:
	UIViewSwitchContainer (const CRect& size);
	~UIViewSwitchContainer ();

	IViewSwitchController* getController () const { return controller; }
	void setController (IViewSwitchController* controller);	// owns controller if it is a CBaseObject

	void setCurrentViewIndex (int32_t viewIndex);
	int32_t getCurrentViewIndex () const { return currentViewIndex; }

	// overrides, views can not be directly added or removed, use the controller
//	bool addView (CView *pView) { return false; }
//	bool addView (CView *pView, CRect &mouseableArea, bool mouseEnabled = true) { return false; }
//	bool addView (CView *pView, CView* pBefore) { return false; }
//	bool removeView (CView *pView, bool withForget = true) { return false; }
//	bool removeAll (bool withForget = true) { return false; }

//-----------------------------------------------------------------------------
	CLASS_METHODS (UIViewSwitchContainer, CViewContainer)
protected:
	bool attached (CView* parent);
	bool removed (CView* parent);

	IViewSwitchController* controller;
	int32_t currentViewIndex;
};

//-----------------------------------------------------------------------------
class IViewSwitchController
{
public:
	IViewSwitchController (UIViewSwitchContainer* viewSwitch) : viewSwitch (viewSwitch) {}
	virtual ~IViewSwitchController () {}

	void init () { viewSwitch->setController (this); }

	UIViewSwitchContainer* getViewSwitchContainer () const { return viewSwitch; }

	virtual CView* createViewForIndex (int32_t index) = 0;
	virtual void switchContainerAttached () {}
protected:
	UIViewSwitchContainer* viewSwitch;
};

//-----------------------------------------------------------------------------
class UIDescriptionViewSwitchController : public CBaseObject, public IViewSwitchController, public CControlListener
{
public:
	UIDescriptionViewSwitchController (UIViewSwitchContainer* viewSwitch, UIDescription* uiDescription, IController* uiController);

	CView* createViewForIndex (int32_t index);
	void switchContainerAttached ();

	void valueChanged (CControl* pControl);

	void setTemplateNames (UTF8StringPtr templateNames); // comma separated
	void getTemplateNames (std::string& str); // comma separated

	void setSwitchControlTag (int32_t tag) { switchControlTag = tag; }
	int32_t getSwitchControlTag () const { return switchControlTag; }
protected:
	UIDescription* uiDescription;
	IController* uiController;
	int32_t switchControlTag;
	int32_t currentIndex;
	std::vector<std::string> templateNames;
};

} // namespace

#endif // __uiviewswitchcontainer__