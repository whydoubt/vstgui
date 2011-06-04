//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
//
// Version 4.0
//
// CDataBrowser written 2006 by Arne Scheffler
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

#include "cdatabrowser.h"
#include "vstkeycode.h"
#include "cframe.h"
#include "controls/ctextedit.h"
#include "ifocusdrawing.h"
#include "cvstguitimer.h"
#include <cmath>
#include <algorithm>

namespace VSTGUI {

/// @cond ignore
//-----------------------------------------------------------------------------------------------
class CDataBrowserView : public CView, public IFocusDrawing
//-----------------------------------------------------------------------------------------------
{
public:
	CDataBrowserView (const CRect& size, IDataBrowser* db, CDataBrowser* browser, int32_t style);

	void draw (CDrawContext* context);
	void drawRect (CDrawContext* context, const CRect& updateRect);
	CMouseEventResult onMouseDown (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseMoved (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseUp (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseExited (CPoint &where, const CButtonState& buttons);
	
	int32_t onKeyDown (VstKeyCode& keyCode);

	CRect getRowBounds (int32_t row);
	void invalidateRow (int32_t row);

	bool getCell (CPoint& where, int32_t& row, int32_t& column);

	bool drawFocusOnTop ();
	bool getFocusPath (CGraphicsPath& outPath);
protected:

	IDataBrowser* db;
	CDataBrowser* browser;
	int32_t style;
};

//-----------------------------------------------------------------------------------------------
class CDataBrowserHeader : public CView
//-----------------------------------------------------------------------------------------------
{
public:
	CDataBrowserHeader (const CRect& size, IDataBrowser* db, CDataBrowser* browser, int32_t style);

	void draw (CDrawContext* context);
	void drawRect (CDrawContext* context, const CRect& updateRect);
	CMouseEventResult onMouseDown (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseMoved (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseUp (CPoint &where, const CButtonState& buttons);
	CMouseEventResult onMouseExited (CPoint &where, const CButtonState& buttons);

protected:
	int32_t getColumnAtPoint (CPoint& where);

	IDataBrowser* db;
	CDataBrowser* browser;
	int32_t style;

	CPoint startMousePoint;
	int32_t mouseColumn;
	CCoord startWidth;
};
/// @endcond

//-----------------------------------------------------------------------------------------------
/**
 * @param size size of data browser
 * @param pParent frame
 * @param db data browser interface. If db is inheritated from CBaseObject it will be remembered and released if data browser is destroyed
 * @param style data browser and scroll view style see #CDataBrowserStyle and #CScrollViewStyle
 * @param scrollbarWidth width of scrollbars
 * @param pBackground background bitmap
 */
CDataBrowser::CDataBrowser (const CRect& size, CFrame* pParent, IDataBrowser* db, int32_t style, CCoord scrollbarWidth, CBitmap* pBackground)
: CScrollView (size, CRect (0, 0, 0, 0), pParent, style, scrollbarWidth, pBackground)
, db (db)
, dbView (0)
, dbHeader (0)
, dbHeaderContainer (0)
, selectedRow (kNoSelection)
{
	setTransparency (true);
	dbView = new CDataBrowserView (CRect (0, 0, 0, 0), db, this, style);
	dbView->setAutosizeFlags (kAutosizeLeft|kAutosizeRight|kAutosizeBottom);
	addView (dbView);
	CBaseObject* obj = dynamic_cast<CBaseObject*>(db);
	if (obj)
		obj->remember ();
}

//-----------------------------------------------------------------------------------------------
CDataBrowser::~CDataBrowser ()
{
	CBaseObject* obj = dynamic_cast<CBaseObject*>(db);
	if (obj)
		obj->forget ();
}

//-----------------------------------------------------------------------------------------------
void CDataBrowser::setAutosizeFlags (int32_t flags)
{
	CScrollView::setAutosizeFlags (flags);
}

//-----------------------------------------------------------------------------------------------
void CDataBrowser::setViewSize (const CRect& size, bool invalid)
{
	CScrollView::setViewSize (size, invalid);
	recalculateLayout (true);
}

//-----------------------------------------------------------------------------------------------
bool CDataBrowser::attached (CView *parent)
{
	recalculateLayout (true);
	bool result = CScrollView::attached (parent);
	if (result && db)
		db->dbAttached (this);
	return result;
}

//-----------------------------------------------------------------------------------------------
bool CDataBrowser::removed (CView* parent)
{
	if (isAttached () && db)
		db->dbRemoved (this);
	return CScrollView::removed (parent);
}

//-----------------------------------------------------------------------------------------------
int32_t CDataBrowser::onKeyDown (VstKeyCode& keyCode)
{
	return dbView ? dbView->onKeyDown (keyCode) : -1;
}

//-----------------------------------------------------------------------------
CMouseEventResult CDataBrowser::onMouseDown (CPoint& where, const CButtonState& buttons)
{
	CMouseEventResult result = CViewContainer::onMouseDown (where, buttons);
	CView* focusView = getFrame ()->getFocusView ();
	if (focusView != dbView && !isChild (focusView, true))
		getFrame ()->setFocusView (dbView);
	return result;
}

//-----------------------------------------------------------------------------
void CDataBrowser::valueChanged (CControl *pControl)
{
	CPoint origOffset = getScrollOffset ();
	CScrollView::valueChanged (pControl);
	CPoint offset = getScrollOffset ();
	if (origOffset != offset)
	{
		switch (pControl->getTag ())
		{
			case kHSBTag:
			{
				if (dbHeader)
				{
					CRect viewSize = dbHeader->getViewSize (viewSize);
					CCoord width = viewSize.getWidth ();
					viewSize.left = offset.x;
					viewSize.setWidth (width);
					dbHeader->setViewSize (viewSize);
					dbHeader->setMouseableArea (viewSize);
					dbHeader->invalid ();
				}
				break;
			}
		}
		if (isAttached () && mouseDownView == dbView || mouseDownView == 0)
		{
			CPoint where;
			getFrame ()->getCurrentMouseLocation (where);
			if (getFrame ()->getViewAt (where, true) == dbView)
			{
				int32_t row = -1;
				int32_t column = -1;
				dbView->frameToLocal (where);
				dbView->getCell (where, row, column);
				db->dbOnMouseMoved (where, getFrame ()->getCurrentMouseButtons (), row, column, this);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------
/**
 * @param rememberSelection if true selection will be remembered
 */
void CDataBrowser::recalculateLayout (bool rememberSelection)
{
	CCoord lineWidth = 0;
	CColor lineColor;
	db->dbGetLineWidthAndColor (lineWidth, lineColor, this);
	CCoord rowHeight = db->dbGetRowHeight (this);
	int32_t numRows = db->dbGetNumRows (this);
	int32_t numColumns = db->dbGetNumColumns (this);
	CCoord allRowsHeight = rowHeight * numRows;
	if (style & kDrawRowLines)
		allRowsHeight += numRows * lineWidth;
	CCoord allColumnsWidth = 0;
	for (int32_t i = 0; i < numColumns; i++)
		allColumnsWidth += db->dbGetCurrentColumnWidth (i, this);
	if (style & kDrawColumnLines)
		allColumnsWidth += numColumns * lineWidth;
	if (style & kDrawHeader)
	{
		FOREACHSUBVIEW
			CRect viewSize;
			pV->getViewSize (viewSize);
			if (pV != dbHeaderContainer && viewSize.top < rowHeight+lineWidth)
			{
				viewSize.top += rowHeight+lineWidth;
				pV->setViewSize (viewSize);
				pV->setMouseableArea (viewSize);
			}
		ENDFOREACHSUBVIEW
	}

	CRect newContainerSize (0, 0, allColumnsWidth, allRowsHeight);
	if (style & kDrawHeader)
	{
		newContainerSize.offset (0, rowHeight+lineWidth);

		CRect headerSize (0, 0, newContainerSize.getWidth (), rowHeight+lineWidth);
		if (style & kHorizontalScrollbar && hsb)
			headerSize.right += hsb->getWidth ();
		if (dbHeader == 0)
		{
			CRect hcs (headerSize);
			if (!(style & kDontDrawFrame))
				hcs.left = hcs.top = 1;
			hcs.setWidth (size.getWidth () - ((style & kDontDrawFrame) ? 0 : 2));
			dbHeaderContainer = new CViewContainer (hcs, getFrame ());
			dbHeaderContainer->setAutosizeFlags (kAutosizeLeft|kAutosizeRight|kAutosizeTop);
			dbHeaderContainer->setTransparency (true);
			headerSize.offset (-headerSize.left, -headerSize.top);
			dbHeader = new CDataBrowserHeader (headerSize, db, this, style);
			dbHeader->setAutosizeFlags (kAutosizeLeft|kAutosizeRight|kAutosizeTop);
			dbHeaderContainer->addView (dbHeader);
			CViewContainer::addView (dbHeaderContainer);
		}
		else
		{
			headerSize.offset (getScrollOffset ().x, 0);
			dbHeader->setViewSize (headerSize);
			dbHeader->setMouseableArea (headerSize);
		}
	}
	setContainerSize (newContainerSize, true);
	newContainerSize.offset (getScrollOffset ().x, -getScrollOffset ().y);
	dbView->setViewSize (newContainerSize);
	dbView->setMouseableArea (newContainerSize);

	if (isAttached ())
		invalid ();
	if (!rememberSelection || numRows <= selectedRow)
		selectedRow = kNoSelection;
}

//-----------------------------------------------------------------------------------------------
/**
 * @param row row to invalidate
 * @param column column to invalidate
 */
void CDataBrowser::invalidate (int32_t row, int32_t column)
{
	invalidRect (getCellBounds (row, column));
}

//-----------------------------------------------------------------------------------------------
/**
 * @param row row to invalidate
 */
void CDataBrowser::invalidateRow (int32_t row)
{
	dbView->invalidateRow (row);
}

//-----------------------------------------------------------------------------------------------
/**
 * @param row row to make visible
 */
void CDataBrowser::makeRowVisible (int32_t row)
{
	CRect r = dbView->getRowBounds (row);
	makeRectVisible (r);
}

//-----------------------------------------------------------------------------------------------
/**
 * @param index row to select
 * @param makeVisible if true makes row visible
 */
void CDataBrowser::setSelectedRow (int32_t index, bool makeVisible)
{
	int32_t numRows = db->dbGetNumRows (this);
	if (index >= numRows)
		index = numRows-1;
	if (index != selectedRow)
	{
		dbView->invalidateRow (selectedRow);
		dbView->invalidateRow (index);
		selectedRow = index;
		db->dbSelectionChanged (this);
	}
	if (makeVisible)
		makeRowVisible (selectedRow);
}

//-----------------------------------------------------------------------------------------------
/**
 * @param row row number
 * @param column column number
 * @return bounds of cell
 */
CRect CDataBrowser::getCellBounds (int32_t row, int32_t column)
{
	CCoord lineWidth = 0;
	if (style & kDrawRowLines || style & kDrawColumnLines)
	{
		CColor lineColor;
		db->dbGetLineWidthAndColor (lineWidth, lineColor, this);
	}
	CCoord rowHeight = db->dbGetRowHeight (this);
	if (style & kDrawRowLines)
		rowHeight += lineWidth;
	CRect result (0, rowHeight * row, 0, rowHeight * (row+1));
	for (int32_t i = 0; i <= column; i++)
	{
		CCoord colWidth = db->dbGetCurrentColumnWidth (i, this);
		if (i != column)
		{
			result.offset (colWidth, 0);
			if (style & kDrawColumnLines)
				result.offset (lineWidth, 0);
		}
		result.setWidth (colWidth);
	}
	CRect viewSize = dbView->getViewSize (viewSize);
	result.offset (viewSize.left, viewSize.top);
	return result;
}

//-----------------------------------------------------------------------------------------------
/**
 * @param row row number
 * @param column column number
 * @param initialText UTF-8 string the text edit field will be initialized with
 */
void CDataBrowser::beginTextEdit (int32_t row, int32_t column, UTF8StringPtr initialText)
{
	CRect r = getCellBounds (row, column);
	makeRectVisible (r);
	CRect cellRect = getCellBounds (row, column);
	CTextEdit* te = new CTextEdit (cellRect, 0, -1, initialText);
	db->dbCellSetupTextEdit(row, column, te, this);
	addView (te);
//	te->takeFocus ();
	getFrame ()->setFocusView (te);
	// save row and column
	te->setAttribute ('row ', sizeof (int32_t), &row);
	te->setAttribute ('col ', sizeof (int32_t), &column);
}

//-----------------------------------------------------------------------------------------------
CMessageResult CDataBrowser::notify (CBaseObject* sender, IdStringPtr message)
{
	if (message == kMsgLooseFocus)
	{
		CTextEdit* te = dynamic_cast<CTextEdit*>(sender);
		if (te)
		{
			// get row and column
			int32_t row = kNoSelection;
			int32_t col = kNoSelection;
			int32_t outSize;
			te->getAttribute ('row ', sizeof (int32_t), &row, outSize);
			te->getAttribute ('col ', sizeof (int32_t), &col, outSize);
			UTF8StringPtr newText = te->getText ();
			db->dbCellTextChanged (row, col, newText, this);
			removeView (te);
			getFrame ()->setFocusView (dbView);
			return kMessageNotified;
		}
	}
	return CViewContainer::notify (sender, message);
}

/// @cond ignore
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CDataBrowserHeader::CDataBrowserHeader (const CRect& size, IDataBrowser* db, CDataBrowser* browser, int32_t style)
: CView (size)
, db (db)
, browser (browser)
, style (style)
{
	setTransparency (true);
}

//-----------------------------------------------------------------------------------------------
void CDataBrowserHeader::draw (CDrawContext* context)
{
	drawRect (context, size);
}

//-----------------------------------------------------------------------------------------------
void CDataBrowserHeader::drawRect (CDrawContext* context, const CRect& updateRect)
{
	CColor lineColor;
	CCoord lineWidth = 0;
	if (style & CDataBrowser::kDrawRowLines || style & CDataBrowser::kDrawColumnLines)
	{
		db->dbGetLineWidthAndColor (lineWidth, lineColor, browser);
	}
	CCoord rowHeight = db->dbGetRowHeight (browser);
	if (style & CDataBrowser::kDrawRowLines)
		rowHeight += lineWidth;
	int32_t numColumns = db->dbGetNumColumns (browser);

	CRect r (size.left, size.top, 0, 0);
	r.setHeight (rowHeight);
	for (int32_t col = 0; col < numColumns; col++)
	{
		CCoord columnWidth = db->dbGetCurrentColumnWidth (col, browser);
		r.setWidth (columnWidth);
		if (style & CDataBrowser::kDrawColumnLines)
			r.right += lineWidth;
		CRect testRect (r);
		testRect.bound (updateRect);
		if (!testRect.isEmpty ())
		{
			db->dbDrawHeader (context, r, col, 0, browser);
		}
		r.offset (r.getWidth (), 0);
	}
	setDirty (false);
}

//-----------------------------------------------------------------------------------------------
int32_t CDataBrowserHeader::getColumnAtPoint (CPoint& where)
{
	// calculate column at point
	CCoord lineWidth = 0;
	if (style & CDataBrowser::kDrawRowLines || style & CDataBrowser::kDrawColumnLines)
	{
		CColor lineColor;
		db->dbGetLineWidthAndColor (lineWidth, lineColor, browser);
	}
	int32_t col = -1;
	int32_t numColumns = db->dbGetNumColumns (browser);
	CRect r (size.left, size.top, 0, size.bottom);
	for (int32_t c = 0; c < numColumns; c++)
	{
		CCoord columnWidth = db->dbGetCurrentColumnWidth (c, browser);
		if (style & CDataBrowser::kDrawColumnLines)
			columnWidth += lineWidth;
		r.setWidth (columnWidth);
		if (r.pointInside (where))
		{
			if (r.right - where.x < 5)
				col = c;
			break;
		}
		r.offset (columnWidth, 0);
	}
	return col;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserHeader::onMouseDown (CPoint &where, const CButtonState& buttons)
{
	mouseColumn = getColumnAtPoint (where);
	CCoord minWidth;
	CCoord maxWidth;
	if (mouseColumn >= 0 && db->dbGetColumnDescription (mouseColumn, minWidth, maxWidth, browser) && minWidth != maxWidth)
	{
		startWidth = db->dbGetCurrentColumnWidth (mouseColumn, browser);
		startMousePoint = where;
		getFrame ()->setCursor (kCursorHSize);
		return onMouseMoved (where, buttons);
	}
	return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserHeader::onMouseMoved (CPoint &where, const CButtonState& buttons)
{
	if (buttons & kLButton)
	{
		CCoord xOffset = where.x - startMousePoint.x;
		if (xOffset != 0)
		{
			CCoord minWidth;
			CCoord maxWidth;
			if (db->dbGetColumnDescription (mouseColumn, minWidth, maxWidth, browser))
			{
				CCoord columnWidth = db->dbGetCurrentColumnWidth (mouseColumn, browser);
				CCoord newWidth = startWidth + xOffset;
				if (newWidth < minWidth)
					newWidth = minWidth;
				if (newWidth > maxWidth)
					newWidth = maxWidth;
				if (newWidth != columnWidth)
				{
					db->dbSetCurrentColumnWidth (mouseColumn, newWidth, browser);
					browser->recalculateLayout (true);
				}
			}
			else
				return kMouseEventNotHandled;
		}
	}
	else
	{
		int32_t col = getColumnAtPoint (where);
		CCoord minWidth;
		CCoord maxWidth;
		if (col >= 0 && db->dbGetColumnDescription (mouseColumn, minWidth, maxWidth, browser) && minWidth != maxWidth)
			getFrame ()->setCursor (kCursorHSize);
		else
			getFrame ()->setCursor (kCursorDefault);
	}
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserHeader::onMouseExited (CPoint &where, const CButtonState& buttons)
{
	getFrame ()->setCursor (kCursorDefault);
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserHeader::onMouseUp (CPoint &where, const CButtonState& buttons)
{
	getFrame ()->setCursor (kCursorDefault);
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
CDataBrowserView::CDataBrowserView (const CRect& size, IDataBrowser* db, CDataBrowser* browser, int32_t style)
: CView (size)
, db (db)
, browser (browser)
, style (style)
{
	setTransparency (true);
	setWantsFocus (true);
}

//-----------------------------------------------------------------------------------------------
CRect CDataBrowserView::getRowBounds (int32_t row)
{
	CCoord lineWidth = 0;
	if (style & CDataBrowser::kDrawRowLines || style & CDataBrowser::kDrawColumnLines)
	{
		CColor lineColor;
		db->dbGetLineWidthAndColor (lineWidth, lineColor, browser);
	}
	CCoord rowHeight = db->dbGetRowHeight (browser);

	CRect where (size);
	where.offset (-size.left, -size.top);
	if (style & CDataBrowser::kDrawRowLines)
		rowHeight+=lineWidth;

	CRect r (size.left, size.top + rowHeight * row, size.right, size.top + rowHeight * (row+1));
	return r;
}

//-----------------------------------------------------------------------------------------------
void CDataBrowserView::invalidateRow (int32_t row)
{
	if (row == CDataBrowser::kNoSelection)
		return;

	CRect ia = getRowBounds (row);
	invalidRect (ia);
}

//-----------------------------------------------------------------------------------------------
void CDataBrowserView::draw (CDrawContext* context)
{
	drawRect (context, size);
}

//-----------------------------------------------------------------------------------------------
void CDataBrowserView::drawRect (CDrawContext* context, const CRect& updateRect)
{
	CCoord lineWidth = 0;
	CColor lineColor;
	if (style & CDataBrowser::kDrawRowLines || style & CDataBrowser::kDrawColumnLines)
	{
		db->dbGetLineWidthAndColor (lineWidth, lineColor, browser);
	}
	CCoord rowHeight = db->dbGetRowHeight (browser);
	if (style & CDataBrowser::kDrawRowLines)
		rowHeight += lineWidth;
	int32_t numRows = db->dbGetNumRows (browser);
	int32_t numColumns = db->dbGetNumColumns (browser);

	CRect r (size.left, size.top, 0, 0);
	for (int32_t col = 0; col < numColumns; col++)
	{
		CCoord columnWidth = db->dbGetCurrentColumnWidth (col, browser);
		r.setWidth (columnWidth);
		r.setHeight (size.getHeight ());
		CRect testRect (r);
		testRect.bound (updateRect);
		if (testRect.isEmpty ())
		{
			r.offset (columnWidth, 0);
			if (style & CDataBrowser::kDrawColumnLines)
				r.offset (lineWidth, 0);
			continue;
		}
		r.setHeight (rowHeight);
		for (int32_t row = 0; row < numRows; row++)
		{
			CRect testRect2 (r);
			testRect2.bound (updateRect);
			if (!testRect2.isEmpty ())
				db->dbDrawCell (context, r, row, col, browser->getSelectedRow () == row ? IDataBrowser::kRowSelected : 0, browser);
			r.offset (0, rowHeight);
		}
		r.offset (columnWidth, 0);
		if (style & CDataBrowser::kDrawColumnLines)
		{
			context->setDrawMode (kAliasing);
			context->setLineWidth (lineWidth);
			context->setFrameColor (lineColor);
			context->setLineStyle (kLineSolid);
			context->moveTo (CPoint (r.left + lineWidth/2, size.top));
			context->lineTo (CPoint (r.left + lineWidth/2, size.bottom));
			r.offset (lineWidth, 0);
		}
		r.top = size.top;
	}
	if (style & CDataBrowser::kDrawRowLines)
	{
		context->setDrawMode (kAliasing);
		context->setLineWidth (lineWidth);
		context->setLineStyle (kLineSolid);
		context->setFrameColor (lineColor);
		CRect rr (size.left, size.top, size.right, size.top + rowHeight);
		for (int32_t row = 0; row < numRows; row++)
		{
			context->moveTo (CPoint (rr.left, rr.bottom));
			context->lineTo (CPoint (rr.right, rr.bottom));
			rr.offset (0, rowHeight);
		}
	}
	setDirty (false);
}

//-----------------------------------------------------------------------------------------------
bool CDataBrowserView::getCell (CPoint& where, int32_t& row, int32_t& column)
{
	CCoord lineWidth = 0;
	if (style & CDataBrowser::kDrawRowLines || style & CDataBrowser::kDrawColumnLines)
	{
		CColor lineColor;
		db->dbGetLineWidthAndColor (lineWidth, lineColor, browser);
	}
	CCoord rowHeight = db->dbGetRowHeight (browser);
	int32_t numColumns = db->dbGetNumColumns (browser);

	CPoint _where (where);
	_where.offset (-size.left, -size.top);
	if (style & CDataBrowser::kDrawRowLines)
		rowHeight += lineWidth;
	int32_t rowNum = (int32_t)(_where.y / rowHeight);
	int32_t colNum = 0;
	CCoord cw = 0;
	for (int32_t i = 0; i < numColumns; i++)
	{
		cw += db->dbGetCurrentColumnWidth (i, browser);
		if (style & CDataBrowser::kDrawColumnLines)
			cw += lineWidth;
		if (_where.x < cw)
		{
			colNum = i;
			row = rowNum;
			column = colNum;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserView::onMouseDown (CPoint &where, const CButtonState& buttons)
{
	getFrame ()->setFocusView (this);
	int32_t rowNum = -1;
	int32_t colNum = -1;
	if (getCell (where, rowNum, colNum))
	{
		if (rowNum != browser->getSelectedRow ())
		{
			invalidateRow (rowNum);
			invalidateRow (browser->getSelectedRow ());
			browser->setSelectedRow (rowNum);
		}
		return db->dbOnMouseDown (where, buttons, rowNum, colNum, browser);
	}
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserView::onMouseMoved (CPoint &where, const CButtonState& buttons)
{
	int32_t rowNum = -1;
	int32_t colNum = -1;
	if (getCell (where, rowNum, colNum))
	{
		return db->dbOnMouseMoved (where, buttons, rowNum, colNum, browser);
	}
	return kMouseEventNotHandled;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserView::onMouseUp (CPoint &where, const CButtonState& buttons)
{
	int32_t rowNum = -1;
	int32_t colNum = -1;
	if (getCell (where, rowNum, colNum))
	{
		return db->dbOnMouseUp (where, buttons, rowNum, colNum, browser);
	}
	return kMouseEventNotHandled;
}

//-----------------------------------------------------------------------------------------------
CMouseEventResult CDataBrowserView::onMouseExited (CPoint &where, const CButtonState& buttons)
{
	return db->dbOnMouseMoved (where, buttons, -1, -1, browser);
}

//-----------------------------------------------------------------------------------------------
int32_t CDataBrowserView::onKeyDown (VstKeyCode& keyCode)
{
	int32_t res = db->dbOnKeyDown (keyCode, browser);
	if (res != -1)
		return res;
	if (keyCode.virt == VKEY_UP || keyCode.virt == VKEY_DOWN)
	{
		int32_t numRows = db->dbGetNumRows (browser);
		int32_t selRow = browser->getSelectedRow ();
		if ((selRow > 0 && keyCode.virt == VKEY_UP) || (selRow < numRows-1 && keyCode.virt == VKEY_DOWN))
		{
			invalidateRow (selRow);
			selRow += keyCode.virt == VKEY_UP ? -1 : 1;
			invalidateRow (selRow);
			browser->setSelectedRow (selRow, true);
			CRect rect = getRowBounds (selRow);
			
			browser->makeRectVisible (rect);
		}
		return 1;
	}
	return -1;
}

//-----------------------------------------------------------------------------------------------
bool CDataBrowserView::drawFocusOnTop ()
{
	return true;
}

//-----------------------------------------------------------------------------------------------
bool CDataBrowserView::getFocusPath (CGraphicsPath& outPath)
{
	CRect r = getVisibleSize ();
	outPath.addRect (r);
	r.inset (0.6, 0.6);
	outPath.addRect (r);
	return true;
}

/// @endcond

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
GenericStringListDataBrowserSource::GenericStringListDataBrowserSource (const std::vector<std::string>* stringList, IGenericStringListDataBrowserSourceSelectionChanged* delegate)
: stringList (stringList)
, rowHeight (-1)
, fontColor (kWhiteCColor)
, selectionColor (kBlueCColor)
, rowlineColor (kGreyCColor)
, rowBackColor (kTransparentCColor)
, rowAlternateBackColor (kTransparentCColor)
, drawFont (kSystemFont)
, dataBrowser (0)
, delegate (delegate)
, timer (0)
{
	drawFont->remember ();
}

//-----------------------------------------------------------------------------
GenericStringListDataBrowserSource::~GenericStringListDataBrowserSource ()
{
	if (timer)
		timer->forget ();
	drawFont->forget ();
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::dbAttached (CDataBrowser* browser)
{
	dataBrowser = browser;
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::dbRemoved (CDataBrowser* browser)
{
	dataBrowser = 0;
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::setStringList (const std::vector<std::string>* stringList)
{
	this->stringList = stringList;
	if (dataBrowser)
		dataBrowser->recalculateLayout (false);
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::setupUI (const CColor& _selectionColor, const CColor& _fontColor, const CColor& _rowlineColor, const CColor& _rowBackColor, const CColor& _rowAlternateBackColor, CFontRef _font, int32_t _rowHeight)
{
	if (_font)
	{
		if (drawFont)
			drawFont->forget ();
		drawFont = _font;
		drawFont->remember ();
	}
	rowHeight = _rowHeight;
	selectionColor = _selectionColor;
	fontColor = _fontColor;
	rowlineColor = _rowlineColor;
	rowBackColor = _rowBackColor;
	rowAlternateBackColor = _rowAlternateBackColor;
	if (dataBrowser)
		dataBrowser->recalculateLayout (true);
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::dbSelectionChanged (CDataBrowser* browser)
{
	if (delegate)
		delegate->dbSelectionChanged (browser->getSelectedRow (), this);
}

//-----------------------------------------------------------------------------
int32_t GenericStringListDataBrowserSource::dbGetNumRows (CDataBrowser* browser)
{
	return (int32_t)stringList->size ();
}

//-----------------------------------------------------------------------------
CCoord GenericStringListDataBrowserSource::dbGetCurrentColumnWidth (int32_t index, CDataBrowser* browser)
{
	return browser->getWidth () - browser->getScrollbarWidth ();
}

//-----------------------------------------------------------------------------
bool GenericStringListDataBrowserSource::dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser)
{
	width = 1.;
	color = rowlineColor;
	return true;
}

//-----------------------------------------------------------------------------
CCoord GenericStringListDataBrowserSource::dbGetRowHeight (CDataBrowser* browser)
{
	if (rowHeight < 0)
	{
		if (drawFont->getPlatformFont ())
		{
			CCoord height = drawFont->getPlatformFont ()->getAscent ();
			height += drawFont->getPlatformFont ()->getDescent ();
			height += drawFont->getPlatformFont ()->getLeading ();
			return ceil (height + 2.);
		}
		return drawFont->getSize () + 2.;
	}
	return rowHeight;
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::dbDrawHeader (CDrawContext* context, const CRect& size, int32_t column, int32_t flags, CDataBrowser* browser)
{
}

//-----------------------------------------------------------------------------
void GenericStringListDataBrowserSource::dbDrawCell (CDrawContext* context, const CRect& size, int32_t row, int32_t column, int32_t flags, CDataBrowser* browser)
{
	context->setFillColor (row % 2 ? rowBackColor : rowAlternateBackColor);
	context->drawRect (size, kDrawFilled);
	if (flags & kRowSelected)
	{
		CColor color (selectionColor);
		CView* focusView = browser->getFrame ()->getFocusView ();
		if (!(focusView && browser->isChild (focusView, true)))
		{
			double hue, saturation, value;
			color.toHSV (hue, saturation, value);
			saturation = 0.;
			color.fromHSV (hue, saturation, value);
			color.alpha /= 3;
		}
		context->setFillColor (color);
		context->drawRect (size, kDrawFilled);
	}
	CRect stringSize (size);
	stringSize.inset (2., 0.);
	context->setFont (drawFont);
	context->setFontColor (fontColor);
	context->drawString ((*stringList)[row].c_str (), stringSize, kLeftText);
}

//-----------------------------------------------------------------------------
int32_t GenericStringListDataBrowserSource::dbOnKeyDown (const VstKeyCode& _key, CDataBrowser* browser)
{
	VstKeyCode key = _key;
	if (key.virt == VKEY_SPACE)
	{
		key.virt = 0;
		key.character = 0x20;
	}
	if (dataBrowser && key.virt == 0 && key.modifier == 0)
	{
		if (timer == 0)
		{
			timer = new CVSTGUITimer (this, 1000);
			timer->start ();
		}
		else
		{
			timer->stop ();
			timer->start ();
		}
		keyDownFindString += toupper (key.character);
		std::vector<std::string>::const_iterator it = stringList->begin ();
		int32_t row = 0;
		while (it != stringList->end ())
		{
			std::string str ((*it), 0, keyDownFindString.length ());
			std::transform (str.begin(), str.end(), str.begin(), ::toupper);
			if (str == keyDownFindString)
			{
				dataBrowser->setSelectedRow (row, true);
				return 1;
			}
			row++;
			it++;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
CMessageResult GenericStringListDataBrowserSource::notify (CBaseObject* sender, IdStringPtr message)
{
	if (message == CVSTGUITimer::kMsgTimer)
	{
		keyDownFindString = "";
		timer->forget ();
		timer = 0;
		return kMessageNotified;
	}
	return kMessageUnknown;
}

} // namespace
