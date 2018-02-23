/*
 * Copyright (C) 2018 Werner Turing <werner.turing@protonmail.com>
 *
 * This file is part of multi-delogo.
 *
 * multi-delogo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * multi-delogo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with multi-delogo.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <memory>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "common/FrameProvider.hpp"

#include "filter-generator/FilterData.hpp"

#include "MovieWindow.hpp"
#include "FilterList.hpp"
#include "FrameNavigator.hpp"
#include "Coordinator.hpp"
#include "MultiDelogoApp.hpp"
#include "EncodeWindow.hpp"

using namespace mdl;


MovieWindow::MovieWindow(const std::string& project_file,
                         std::unique_ptr<fg::FilterData> filter_data,
                         const Glib::RefPtr<FrameProvider>& frame_provider)
  : project_file_(project_file)
  , filter_data_(std::move(filter_data))
  , filter_list_(filter_data_->filter_list())
  , frame_navigator_(*this, frame_provider)
  , coordinator_(filter_list_, frame_navigator_,
                 frame_provider->get_frame_width(), frame_provider->get_frame_height())
{
  set_default_size(1000, 600);
  set_title(Glib::ustring::compose("multi-delogo: %1",
                                   Glib::path_get_basename(project_file)));

  Gtk::Box* vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
  vbox->pack_start(*create_toolbar(), false, false);

  Gtk::Box* hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
  hbox->set_border_width(6);
  hbox->pack_start(filter_list_, false, false);
  hbox->pack_start(frame_navigator_, true, true);

  vbox->pack_start(*hbox, true, true);

  add(*vbox);

  frame_navigator_.set_jump_size(filter_data_->jump_size());

  signal_key_press_event().connect(sigc::mem_fun(*this, &MovieWindow::on_key_press));
}


Gtk::Toolbar* MovieWindow::create_toolbar()
{
  Gtk::ToolButton* btn_new = Gtk::manage(new Gtk::ToolButton());
  btn_new->set_tooltip_text(_("Create a new project"));
  btn_new->set_icon_name("document-new");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_new->gobj()), MultiDelogoApp::ACTION_NEW.c_str());

  Gtk::ToolButton* btn_open = Gtk::manage(new Gtk::ToolButton());
  btn_open->set_tooltip_text(_("Open an existing project"));
  btn_open->set_icon_name("document-open");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_open->gobj()), MultiDelogoApp::ACTION_OPEN.c_str());

  add_action("save", sigc::mem_fun(*this, &MovieWindow::on_save));
  Gtk::ToolButton* btn_save = Gtk::manage(new Gtk::ToolButton());
  btn_save->set_tooltip_text(_("Save current project"));
  btn_save->set_icon_name("document-save");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_save->gobj()), "win.save");

  add_action("encode", sigc::mem_fun(*this, &MovieWindow::on_encode));
  Gtk::ToolButton* btn_encode = Gtk::manage(new Gtk::ToolButton(_("Encode")));
  btn_encode->set_tooltip_text(_("Encode current project to a video with the filters applied"));
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn_encode->gobj()), "win.encode");

  Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar());
  toolbar->append(*btn_new);
  toolbar->append(*btn_open);
  toolbar->append(*btn_save);
  toolbar->append(*Gtk::manage(new Gtk::SeparatorToolItem()));
  toolbar->append(*btn_encode);
  return toolbar;
}


bool MovieWindow::on_key_press(GdkEventKey* key_event)
{
  switch (key_event->keyval) {
  case GDK_KEY_A:
  case GDK_KEY_a:
    frame_navigator_.jump_step_frame(-1);
    return true;

  case GDK_KEY_S:
  case GDK_KEY_s:
    frame_navigator_.single_step_frame(-1);
    return true;

  case GDK_KEY_D:
  case GDK_KEY_d:
    frame_navigator_.single_step_frame(1);
    return true;

  case GDK_KEY_F:
  case GDK_KEY_f:
    frame_navigator_.jump_step_frame(1);
    return true;
  }

  return false;
}


void MovieWindow::on_save()
{
  coordinator_.update_current_filter_if_necessary();
  filter_data_->set_jump_size(frame_navigator_.get_jump_size());
  get_application()->save_project(project_file_, *filter_data_);
}


void MovieWindow::on_encode()
{
  on_save();

  EncodeWindow* window = new EncodeWindow(std::move(filter_data_));
  get_application()->register_window(window);

  hide();
}


void MovieWindow::on_hide()
{
  // When this is called because of on_encode there is no filter_data_ anymore
  if (filter_data_) {
    on_save();
  }
}


Glib::RefPtr<MultiDelogoApp> MovieWindow::get_application()
{
  Glib::RefPtr<MultiDelogoApp> app
    = Glib::RefPtr<MultiDelogoApp>::cast_dynamic(ApplicationWindow::get_application());
  return app;
}


Glib::RefPtr<const MultiDelogoApp> MovieWindow::get_application() const
{
  Glib::RefPtr<const MultiDelogoApp> app
    = Glib::RefPtr<const MultiDelogoApp>::cast_dynamic(ApplicationWindow::get_application());
  return app;
}
