#ifndef MODAL_HPP
#define MODAL_HPP

#include <cstddef>
#include <deque>
// #include <touchgfx/widgets/Button.hpp>
#include <touchgfx/containers/ModalWindow.hpp>
#include <touchgfx/containers/buttons/Buttons.hpp>
#include <touchgfx/containers/buttons/ImageButtonStyle.hpp>
#include <touchgfx/widgets/TextArea.hpp>

namespace modal
{

struct ModalElements
{
  touchgfx::ModalWindow *modalWindowInformation;
  touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>
      *modalWindowInformationClose;
  touchgfx::TextArea *modalWindowInformationBody;
  touchgfx::TextArea *modalWindowInformationTitle;
  touchgfx::ModalWindow *modalWindowDecision;
  touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>
      *modalWindowDecisionYes;
  touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger>
      *modalWindowDecisionNo;
  touchgfx::TextArea *modalWindowDecisionBody;
  touchgfx::TextArea *modalWindowDecisionTitle;
  // OK of the information modal while it can't be pressed yet
  touchgfx::Bitmap modalWindowInformationCloseReleased;
  touchgfx::Bitmap modalWindowInformationCloseDisabled;
};

typedef void (*ModalInformationCallback)();
typedef void (*ModalDecisionCallback)(bool answer);

enum class ShowInformationType
{
  INFO,
  INFO_NON_QUITTABLE,
  DECISION,
};

struct ShowInformation
{
  ShowInformationType type;
  touchgfx::TypedText title;
  touchgfx::TypedText body;

  ModalInformationCallback informationCallback;
  ModalDecisionCallback decisionCallback;

  bool okEnabled;
};

class Modal
{
  ModalElements elements;
  std::deque<ShowInformation> sheduledModalDisplays;

  bool showing = false;
  ShowInformation activeModalInformation;

  void showModal(const ShowInformation &info)
  {
    showing = true;
    activeModalInformation = info;

    elements.modalWindowDecision->setVisible(false);
    elements.modalWindowInformation->setVisible(false);

    if (info.type == ShowInformationType::INFO ||
        info.type == ShowInformationType::INFO_NON_QUITTABLE)
    {
      elements.modalWindowInformationTitle->setTypedText(
          activeModalInformation.title);
      elements.modalWindowInformationBody->setTypedText(
          activeModalInformation.body);
      elements.modalWindowInformation->setVisible(true);
      elements.modalWindowInformationClose->setVisible(
          info.type == ShowInformationType::INFO);
      applyInformationOkEnabled(info.okEnabled);
    }
    else
    {
      elements.modalWindowDecisionTitle->setTypedText(
          activeModalInformation.title);
      elements.modalWindowDecisionBody->setTypedText(
          activeModalInformation.body);
      elements.modalWindowDecision->setVisible(true);
    }

    elements.modalWindowDecision->invalidate();
    elements.modalWindowInformation->invalidate();
  }

  void applyInformationOkEnabled(bool enabled)
  {
    touchgfx::ImageButtonStyle<touchgfx::TouchButtonTrigger> *ok =
        elements.modalWindowInformationClose;

    if (ok->isTouchable() == enabled)
    {
      return;
    }

    ok->setTouchable(enabled);
    ok->setBitmaps(enabled ? elements.modalWindowInformationCloseReleased
                           : elements.modalWindowInformationCloseDisabled,
                   elements.modalWindowInformationCloseDisabled);
    ok->invalidate();
  }

  static bool isInformationModal(const ShowInformation &info,
                                 ModalInformationCallback callback)
  {
    return info.type == ShowInformationType::INFO &&
           info.informationCallback == callback;
  }

  void hideModal()
  {
    elements.modalWindowDecision->setVisible(false);
    elements.modalWindowDecision->invalidate();

    elements.modalWindowInformation->setVisible(false);
    elements.modalWindowInformation->invalidate();
  }

  void showNextModal()
  {
    if (sheduledModalDisplays.empty())
    {
      showing = false;
      return;
    }

    ShowInformation next = sheduledModalDisplays.front();
    sheduledModalDisplays.pop_front();
    showModal(next);
  }

  void pushInfo(const ShowInformation &info)
  {
    if (showing)
    {
      sheduledModalDisplays.push_back(info);
      return;
    }

    showModal(info);
  }

  void executeCallback(bool answer)
  {
    if (activeModalInformation.type == ShowInformationType::DECISION)
    {
      if (activeModalInformation.decisionCallback != NULL)
      {
        activeModalInformation.decisionCallback(answer);
      }
    }
    else
    {
      if (activeModalInformation.informationCallback != NULL)
      {
        activeModalInformation.informationCallback();
      }
    }
  }

public:
  Modal() {}

  Modal(ModalElements elements) { this->elements = elements; }

  /* okEnabled false: OK can't be pressed until setInformationOkEnabled() */
  void sheduleInformationModal(touchgfx::TypedText title,
                               touchgfx::TypedText body,
                               ModalInformationCallback callback = NULL,
                               bool okEnabled = true)
  {
    ShowInformation info = {.type = ShowInformationType::INFO,
                            .title = title,
                            .body = body,
                            .informationCallback = callback,
                            .decisionCallback = NULL,
                            .okEnabled = okEnabled};

    pushInfo(info);
  }

  void sheduleDecisionModal(touchgfx::TypedText title, touchgfx::TypedText body,
                            ModalDecisionCallback callback = NULL)
  {
    ShowInformation info = {.type = ShowInformationType::DECISION,
                            .title = title,
                            .body = body,
                            .informationCallback = NULL,
                            .decisionCallback = callback,
                            .okEnabled = true};

    pushInfo(info);
  }

  void sheduleNonQuittableModal(touchgfx::TypedText title,
                                touchgfx::TypedText body)
  {
    ShowInformation info = {.type = ShowInformationType::INFO_NON_QUITTABLE,
                            .title = title,
                            .body = body,
                            .informationCallback = NULL,
                            .decisionCallback = NULL,
                            .okEnabled = true};

    pushInfo(info);
  }

  /* Enables or disables OK of the information modals with this callback,
   * shown or still sheduled */
  void setInformationOkEnabled(ModalInformationCallback callback, bool enabled)
  {
    for (ShowInformation &info : sheduledModalDisplays)
    {
      if (isInformationModal(info, callback))
      {
        info.okEnabled = enabled;
      }
    }

    if (showing && isInformationModal(activeModalInformation, callback) &&
        activeModalInformation.okEnabled != enabled)
    {
      activeModalInformation.okEnabled = enabled;
      applyInformationOkEnabled(enabled);
    }
  }

  /* Removes the information modals with this callback, shown or still
   * sheduled, without calling it */
  void cancelInformationModal(ModalInformationCallback callback)
  {
    for (auto it = sheduledModalDisplays.begin();
         it != sheduledModalDisplays.end();)
    {
      it = isInformationModal(*it, callback) ? sheduledModalDisplays.erase(it)
                                             : it + 1;
    }

    if (showing && isInformationModal(activeModalInformation, callback))
    {
      hideModal();
      showNextModal();
    }
  }

  void closeModal(bool answer)
  {
    hideModal();
    executeCallback(answer);
    showNextModal();
  }

  /* display info, but only if no other modal is open, not quittable by user */
  void showOverlayInfo(touchgfx::TypedText title, touchgfx::TypedText body)
  {
    if (showing)
    {
      return;
    }

    elements.modalWindowInformationTitle->setTypedText(title);
    elements.modalWindowInformationBody->setTypedText(body);
    elements.modalWindowInformation->setVisible(true);
    elements.modalWindowInformationClose->setVisible(false);

    elements.modalWindowInformation->invalidate();
  }

  void hideOverlayInfo()
  {
    if (showing)
    {
      return;
    }

    elements.modalWindowInformation->setVisible(false);
    elements.modalWindowInformation->invalidate();
  }
};

} // namespace modal

#endif
