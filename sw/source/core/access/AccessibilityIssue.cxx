/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>

#include <comphelper/propertyvalue.hxx>
#include <comphelper/dispatchcommand.hxx>

#include <AccessibilityIssue.hxx>
#include <AccessibilityCheckStrings.hrc>
#include <drawdoc.hxx>
#include <edtwin.hxx>
#include <IDocumentDrawModelAccess.hxx>
#include <OnlineAccessibilityCheck.hxx>
#include <swtypes.hxx>
#include <wrtsh.hxx>
#include <docsh.hxx>
#include <view.hxx>
#include <comphelper/lok.hxx>
#include <cui/dlgname.hxx>
#include <svx/svdpage.hxx>
#include <svx/svxdlg.hxx>

#include <svx/svdview.hxx>
#include <flyfrm.hxx>
#include <txatbase.hxx>
#include <txtfrm.hxx>

namespace sw
{
AccessibilityIssue::AccessibilityIssue(sfx::AccessibilityIssueID eIssueID)
    : sfx::AccessibilityIssue(eIssueID)
    , m_eIssueObject(IssueObject::UNKNOWN)
    , m_pDoc(nullptr)
    , m_pNode(nullptr)
    , m_pTextFootnote(nullptr)
    , m_nStart(0)
    , m_nEnd(0)
{
}

void AccessibilityIssue::setIssueObject(IssueObject eIssueObject) { m_eIssueObject = eIssueObject; }

void AccessibilityIssue::setDoc(SwDoc& rDoc) { m_pDoc = &rDoc; }

void AccessibilityIssue::setObjectID(OUString const& rID) { m_sObjectID = rID; }

bool AccessibilityIssue::canGotoIssue() const
{
    if (m_pDoc && m_eIssueObject != IssueObject::UNKNOWN
        && m_eIssueObject != IssueObject::DOCUMENT_TITLE
        && m_eIssueObject != IssueObject::DOCUMENT_BACKGROUND
        && m_eIssueObject != IssueObject::LANGUAGE_NOT_SET)
        return true;
    return false;
}

void AccessibilityIssue::gotoIssue() const
{
    if (!m_pDoc)
        return;

    /* Copying the issueobject because the EnterSelFrameMode ends up calling some sidebar functions
    that recreate the list of a11y issues and the AccessibilityIssue objects are stored by value in a vector
    and the vector is being mutated there and so the instance is overwritten with something else. */
    AccessibilityIssue TempIssueObject(*this);

    SwWrtShell* pWrtShell = TempIssueObject.m_pDoc->GetDocShell()->GetWrtShell();

    pWrtShell->AssureStdMode();

    switch (TempIssueObject.m_eIssueObject)
    {
        case IssueObject::LINKED:
        case IssueObject::GRAPHIC:
        case IssueObject::OLE:
        case IssueObject::TEXTFRAME:
        {
            bool bSelected = pWrtShell->GotoFly(TempIssueObject.m_sObjectID, FLYCNTTYPE_ALL, true);

            // bring issue to attention
            if (bSelected)
            {
                if (const SwFlyFrameFormat* pFlyFormat
                    = m_pDoc->FindFlyByName(TempIssueObject.m_sObjectID, SwNodeType::NONE))
                {
                    if (SwFlyFrame* pFlyFrame
                        = SwIterator<SwFlyFrame, SwFormat>(*pFlyFormat).First())
                    {
                        pWrtShell->GetView().BringToAttention(pFlyFrame->getFrameArea().SVRect());
                    }
                }
            }

            if (bSelected && pWrtShell->IsFrameSelected())
            {
                pWrtShell->HideCursor();
                pWrtShell->EnterSelFrameMode();
            }

            if (!bSelected && TempIssueObject.m_eIssueObject == IssueObject::TEXTFRAME)
            {
                pWrtShell->GotoDrawingObject(TempIssueObject.m_sObjectID);

                // bring issue to attention
                if (SdrPage* pPage
                    = pWrtShell->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0))
                {
                    if (SdrObject* pObj = pPage->GetObjByName(TempIssueObject.m_sObjectID))
                    {
                        pWrtShell->GetView().BringToAttention(pObj->GetLogicRect());
                    }
                }
            }
            if (comphelper::LibreOfficeKit::isActive())
                pWrtShell->ShowCursor();
        }
        break;
        case IssueObject::SHAPE:
        {
            if (pWrtShell->IsFrameSelected())
                pWrtShell->LeaveSelFrameMode();
            pWrtShell->GotoDrawingObject(TempIssueObject.m_sObjectID);

            // bring issue to attention
            if (SdrPage* pPage
                = pWrtShell->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0))
            {
                if (SdrObject* pObj = pPage->GetObjByName(TempIssueObject.m_sObjectID))
                {
                    pWrtShell->GetView().BringToAttention(pObj->GetLogicRect());
                }
            }

            if (comphelper::LibreOfficeKit::isActive())
                pWrtShell->ShowCursor();
        }
        break;
        case IssueObject::FORM:
        {
            bool bIsDesignMode = pWrtShell->GetView().GetFormShell()->IsDesignMode();
            if (bIsDesignMode || (!bIsDesignMode && pWrtShell->WarnSwitchToDesignModeDialog()))
            {
                if (!bIsDesignMode)
                    pWrtShell->GetView().GetFormShell()->SetDesignMode(true);
                pWrtShell->GotoDrawingObject(TempIssueObject.m_sObjectID);

                // bring issue to attention
                if (SdrPage* pPage
                    = pWrtShell->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0))
                {
                    if (SdrObject* pObj = pPage->GetObjByName(TempIssueObject.m_sObjectID))
                    {
                        pWrtShell->GetView().BringToAttention(pObj->GetLogicRect());
                    }
                }

                if (comphelper::LibreOfficeKit::isActive())
                    pWrtShell->ShowCursor();
            }
        }
        break;
        case IssueObject::TABLE:
        {
            pWrtShell->GotoTable(TempIssueObject.m_sObjectID);

            // bring issue to attention
            if (SwTable* pTmpTable = SwTable::FindTable(
                    TempIssueObject.m_pDoc->FindTableFormatByName(TempIssueObject.m_sObjectID)))
            {
                if (SwTableNode* pTableNode = pTmpTable->GetTableNode())
                {
                    pWrtShell->GetView().BringToAttention(pTableNode);
                }
            }

            if (comphelper::LibreOfficeKit::isActive())
                pWrtShell->ShowCursor();
        }
        break;
        case IssueObject::TEXT:
        {
            SwContentNode* pContentNode = TempIssueObject.m_pNode->GetContentNode();
            SwPosition aPoint(*pContentNode, TempIssueObject.m_nStart);
            SwPosition aMark(*pContentNode, TempIssueObject.m_nEnd);
            pWrtShell->StartAllAction();
            SwPaM* pPaM = pWrtShell->GetCursor();
            *pPaM->GetPoint() = aPoint;
            pPaM->SetMark();
            *pPaM->GetMark() = aMark;
            pWrtShell->EndAllAction();

            // bring issue to attention
            pWrtShell->GetView().BringToAttention(pContentNode);

            if (comphelper::LibreOfficeKit::isActive())
                pWrtShell->ShowCursor();
        }
        break;
        case IssueObject::FOOTENDNOTE:
        {
            if (TempIssueObject.m_pTextFootnote)
            {
                pWrtShell->GotoFootnoteAnchor(*TempIssueObject.m_pTextFootnote);

                // bring issue to attention
                const SwTextNode& rTextNode = TempIssueObject.m_pTextFootnote->GetTextNode();
                if (SwTextFrame* pFrame
                    = static_cast<SwTextFrame*>(rTextNode.getLayoutFrame(pWrtShell->GetLayout())))
                {
                    auto nStart = TempIssueObject.m_pTextFootnote->GetStart();
                    auto nEnd = nStart + 1;
                    SwPosition aStartPos(rTextNode, nStart), aEndPos(rTextNode, nEnd);
                    SwRect aStartCharRect, aEndCharRect;
                    pFrame->GetCharRect(aStartCharRect, aStartPos);
                    pFrame->GetCharRect(aEndCharRect, aEndPos);
                    tools::Rectangle aRect(aStartCharRect.Left() - 50, aStartCharRect.Top(),
                                           aEndCharRect.Right() + 50, aStartCharRect.Bottom());
                    pWrtShell->GetView().BringToAttention(aRect);
                }
            }
            if (comphelper::LibreOfficeKit::isActive())
                pWrtShell->ShowCursor();
        }
        break;
        default:
            break;
    }
    pWrtShell->GetView().GetEditWin().GrabFocus();
}

bool AccessibilityIssue::canQuickFixIssue() const
{
    return m_eIssueObject == IssueObject::GRAPHIC || m_eIssueObject == IssueObject::OLE
           || m_eIssueObject == IssueObject::SHAPE || m_eIssueObject == IssueObject::FORM
           || m_eIssueObject == IssueObject::DOCUMENT_TITLE
           || m_eIssueObject == IssueObject::DOCUMENT_BACKGROUND
           || m_eIssueObject == IssueObject::LANGUAGE_NOT_SET;
}

void AccessibilityIssue::quickFixIssue() const
{
    if (!m_pDoc)
        return;

    if (canGotoIssue())
        gotoIssue();

    switch (m_eIssueObject)
    {
        case IssueObject::GRAPHIC:
        case IssueObject::OLE:
        {
            SwFlyFrameFormat* pFlyFormat
                = const_cast<SwFlyFrameFormat*>(m_pDoc->FindFlyByName(m_sObjectID));
            if (pFlyFormat)
            {
                OUString aDescription(pFlyFormat->GetObjDescription());
                OUString aTitle(pFlyFormat->GetObjTitle());
                bool isDecorative(pFlyFormat->IsDecorative());

                SwWrtShell* pWrtShell = m_pDoc->GetDocShell()->GetWrtShell();
                SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
                ScopedVclPtr<AbstractSvxObjectTitleDescDialog> pDlg(
                    pFact->CreateSvxObjectTitleDescDialog(pWrtShell->GetView().GetFrameWeld(),
                                                          aTitle, aDescription, isDecorative));

                if (pDlg->Execute() == RET_OK)
                {
                    pDlg->GetTitle(aTitle);
                    pDlg->GetDescription(aDescription);
                    pDlg->IsDecorative(isDecorative);

                    m_pDoc->SetFlyFrameTitle(*pFlyFormat, aTitle);
                    m_pDoc->SetFlyFrameDescription(*pFlyFormat, aDescription);
                    m_pDoc->SetFlyFrameDecorative(*pFlyFormat, isDecorative);

                    pWrtShell->SetModified();
                }
            }
        }
        break;
        case IssueObject::SHAPE:
        case IssueObject::FORM:
        {
            SwWrtShell* pWrtShell = m_pDoc->GetDocShell()->GetWrtShell();
            auto pPage = pWrtShell->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0);
            SdrObject* pObj = pPage->GetObjByName(m_sObjectID);
            if (pObj)
            {
                OUString aTitle(pObj->GetTitle());
                OUString aDescription(pObj->GetDescription());
                bool isDecorative(pObj->IsDecorative());

                SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
                ScopedVclPtr<AbstractSvxObjectTitleDescDialog> pDlg(
                    pFact->CreateSvxObjectTitleDescDialog(pWrtShell->GetView().GetFrameWeld(),
                                                          aTitle, aDescription, isDecorative));

                if (RET_OK == pDlg->Execute())
                {
                    pDlg->GetTitle(aTitle);
                    pDlg->GetDescription(aDescription);
                    pDlg->IsDecorative(isDecorative);

                    pObj->SetTitle(aTitle);
                    pObj->SetDescription(aDescription);
                    pObj->SetDecorative(isDecorative);

                    pWrtShell->SetModified();
                }
            }
        }
        break;
        case IssueObject::DOCUMENT_TITLE:
        {
            SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
            SwWrtShell* pWrtShell = m_pDoc->GetDocShell()->GetWrtShell();
            ScopedVclPtr<AbstractSvxNameDialog> aNameDialog(pFact->CreateSvxNameDialog(
                pWrtShell->GetView().GetFrameWeld(), OUString(),
                SwResId(STR_DOCUMENT_TITLE_DLG_DESC), SwResId(STR_DOCUMENT_TITLE_DLG_TITLE)));
            if (aNameDialog->Execute() == RET_OK)
            {
                SwDocShell* pShell = m_pDoc->GetDocShell();
                if (!pShell)
                    return;

                const uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                    pShell->GetModel(), uno::UNO_QUERY_THROW);
                const uno::Reference<document::XDocumentProperties> xDocumentProperties(
                    xDPS->getDocumentProperties());
                OUString sName;
                aNameDialog->GetName(sName);
                xDocumentProperties->setTitle(sName);

                m_pDoc->getOnlineAccessibilityCheck()->resetAndQueueDocumentLevel();
            }
        }
        break;
        case IssueObject::DOCUMENT_BACKGROUND:
        {
            uno::Reference<frame::XModel> xModel(m_pDoc->GetDocShell()->GetModel(),
                                                 uno::UNO_QUERY_THROW);

            comphelper::dispatchCommand(".uno:PageAreaDialog",
                                        xModel->getCurrentController()->getFrame(), {});
        }
        break;
        case IssueObject::LANGUAGE_NOT_SET:
        {
            uno::Reference<frame::XModel> xModel(m_pDoc->GetDocShell()->GetModel(),
                                                 uno::UNO_QUERY_THROW);

            if (m_sObjectID.isEmpty())
            {
                // open the dialog "Tools/Options/Language Settings - Language"
                uno::Sequence<beans::PropertyValue> aArgs{ comphelper::makePropertyValue(
                    "Language", OUString("*")) };

                comphelper::dispatchCommand(".uno:LanguageStatus",
                                            xModel->getCurrentController()->getFrame(), aArgs);
            }
            else
            {
                uno::Sequence<beans::PropertyValue> aArgs{
                    comphelper::makePropertyValue("Param", m_sObjectID),
                    comphelper::makePropertyValue("Family", sal_Int16(SfxStyleFamily::Para))
                };

                comphelper::dispatchCommand(".uno:EditStyleFont",
                                            xModel->getCurrentController()->getFrame(), aArgs);
            }
        }
        break;
        default:
            break;
    }
    if (m_pNode)
        m_pDoc->getOnlineAccessibilityCheck()->resetAndQueue(m_pNode);
}

} // end sw namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
