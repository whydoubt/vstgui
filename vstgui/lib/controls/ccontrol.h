//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
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

#ifndef __ccontrol__
#define __ccontrol__

#include "../cview.h"
#include "../ifocusdrawing.h"
#include <list>

//------------------
// defines
//------------------
#ifndef kPI
#define kPI    3.14159265358979323846
#endif

#ifndef k2PI
#define k2PI   6.28318530717958647692
#endif

#ifndef kPI_2
#define kPI_2  1.57079632679489661923f
#endif

#ifndef kPI_4
#define kPI_4  0.78539816339744830962
#endif

#ifndef kE
#define kE     2.7182818284590452354
#endif

#ifndef kLN2
#define kLN2   0.69314718055994530942
#endif

#ifndef kSQRT2
#define kSQRT2 1.41421356237309504880
#endif

namespace VSTGUI {
class CControl;
} // namespace

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CControlListener
{
public:
	virtual ~CControlListener() {}
	virtual void valueChanged (VSTGUI::CControl* pControl) = 0;
	virtual long controlModifierClicked (VSTGUI::CControl* pControl, long button) { return 0; }	///< return 1 if you want the control to not handle it, otherwise 0
	virtual void controlBeginEdit (VSTGUI::CControl* pControl) {}
	virtual void controlEndEdit (VSTGUI::CControl* pControl) {}
	virtual void controlTagWillChange (VSTGUI::CControl* pControl) {}
	virtual void controlTagDidChange (VSTGUI::CControl* pControl) {}
};

namespace VSTGUI {

//------------------
// CControlEnum type
//------------------
enum CControlEnum
{
	kHorizontal			= 1 << 0,
	kVertical			= 1 << 1,
	kShadowText			= 1 << 2,
	kLeft				= 1 << 3,
	kRight				= 1 << 4,
	kTop				= 1 << 5,
	kBottom				= 1 << 6,
	k3DIn				= 1 << 7,
	k3DOut				= 1 << 8,
	kPopupStyle			= 1 << 9,
	kCheckStyle			= 1 << 10,
	kMultipleCheckStyle,
	kNoTextStyle		= 1 << 11,
	kNoDrawStyle		= 1 << 12,
	kDoubleClickStyle	= 1 << 13,
	kNoFrame			= 1 << 14
};

//-----------------------------------------------------------------------------
// CControl Declaration
//! @brief base class of all VSTGUI controls
//-----------------------------------------------------------------------------
class CControl : public CView, public IFocusDrawing
{
public:
	CControl (const CRect& size, CControlListener* listener = 0, long tag = 0, CBitmap* pBackground = 0);
	CControl (const CControl& c);

	//-----------------------------------------------------------------------------
	/// @name Value Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual void setValue (float val, bool updateSubListeners = false);
	virtual float getValue () const { return value; };

	virtual void setValueNormalized (float val, bool updateSubListeners = false);
	virtual float getValueNormalized () const;

	virtual void setMin (float val) { vmin = val; }
	virtual float getMin () const { return vmin; }
	virtual void setMax (float val) { vmax = val; }
	virtual float getMax () const { return vmax; }

	virtual void setOldValue (float val) { oldValue = val; }
	virtual	float getOldValue (void) const { return oldValue; }
	virtual void setDefaultValue (float val) { defaultValue = val; }
	virtual	float getDefaultValue (void) const { return defaultValue; }

	virtual void bounceValue ();
	virtual bool checkDefaultValue (long button);
	
	virtual void valueChanged ();	///< notifies listeners
	//@}

	//-----------------------------------------------------------------------------
	/// @name Editing Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual void setTag (long val);
	virtual long getTag () const { return tag; }

	virtual void beginEdit ();
	virtual void endEdit ();

	virtual CControlListener* getListener () const { return listener; }	///< get main listener
	virtual void setListener (CControlListener* l) { listener = l; } ///< set main listener
	
	virtual void addListener (CControlListener* l) { listeners.push_back (l); } ///< add sub listener
	virtual void removeListener (CControlListener* l) { listeners.remove (l); } ///< remove sub listener
	const std::list<CControlListener*> getListeners () const { return listeners; } ///< get sub listeners
	//@}

	//-----------------------------------------------------------------------------
	/// @name Misc
	//-----------------------------------------------------------------------------
	//@{
	virtual void setBackOffset (const CPoint& offset);
	virtual const CPoint& getBackOffset () const { return backOffset; }
	virtual void copyBackOffset ();

	virtual void setWheelInc (float val) { wheelInc = val; }
	virtual float getWheelInc () const { return wheelInc; }

	virtual void doIdleStuff ();
	//@}

	// overrides
	void draw (CDrawContext* pContext) = 0;
	bool isDirty () const;
	void setDirty (const bool val = true);

	bool drawFocusOnTop ();
	bool getFocusPath (CGraphicsPath& outPath);

	CLASS_METHODS_VIRTUAL(CControl, CView)
protected:
	~CControl ();

	CControlListener* listener;
	long  tag;
	float oldValue;
	float defaultValue;
	float value;
	float vmin;
	float vmax;
	float wheelInc;

	long lastTicks;

	CPoint	backOffset;
	
	std::list<CControlListener*> listeners;
};

//-----------------------------------------------------------------------------
// IMultiBitmapControl Declaration
//! @brief interface for controls with sub images
//-----------------------------------------------------------------------------
class IMultiBitmapControl
{
public:
	virtual ~IMultiBitmapControl() {}
	virtual void setHeightOfOneImage (const CCoord& height) { heightOfOneImage = height; }
	virtual CCoord getHeightOfOneImage () const { return heightOfOneImage; }

	virtual void setNumSubPixmaps (long numSubPixmaps) { subPixmaps = numSubPixmaps; }
	virtual long getNumSubPixmaps () const { return subPixmaps; }

	virtual void autoComputeHeightOfOneImage ();
protected:
	CCoord heightOfOneImage;
	long subPixmaps;
};

} // namespace

#endif
