#include "MainWindow.h"

#include "Utils.h"

namespace WoohooDX12
{
  bool MainWindow::Create(int width, int height)
  {
    m_width = width;
    m_height = height;

    // Create Window
    xwin::WindowDesc wdesc;
    wdesc.title = "Main Title";
    wdesc.name = "MainWindow";
    wdesc.visible = true;
    wdesc.width = width;
    wdesc.height = height;
    wdesc.fullscreen = false;

    m_eventQueue = new xwin::EventQueue();
    m_window = new xwin::Window();

    if (!m_window->create(wdesc, *m_eventQueue))
    {
      Log("Window creation has failed.", LogType::LT_ERROR);
      return false;
    };

    return true;
  }
  bool MainWindow::Close()
  {
    m_window->close();

    return true;
  }
}
