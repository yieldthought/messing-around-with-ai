/*
   Copyright (C) 2008 - 2013 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-editor"

#include "gui/dialogs/editor/custom_tod.hpp"

#include "dialogs.hpp"
#include "filesystem.hpp"
#include "filechooser.hpp"
#include "editor/editor_preferences.hpp"
#include "editor/editor_display.hpp"
#include "gui/dialogs/field.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/slider.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/image.hpp"
#include "gui/widgets/text_box.hpp"
#include "gettext.hpp"
#include "image.hpp"

#include <boost/bind.hpp>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_custom_tod
 *
 * == Custom Schedules ==
 *
 * This shows the dialog to modify tod schedules.
 *
 * @begin{table}{dialog_widgets}
 *
 * current_tod_name & & text_box & m &
 *         The name of the time of day(ToD). $
 *
 * current_tod_id & & text_box & m &
 *         The id of the time of day(ToD). $
 *
 * current_tod_image & & image & m &
 *         The image for the time of day(ToD). $
 *
 * current_tod_mask & & image & m &
 *         The image mask for the time of day(ToD). $
 *
 * current_tod_sonund & & label & m &
 *         The sound for the time of day(ToD). $
 *
 * next_tod & & button & m &
 *         Selects the next ToD. $
 *
 * prev_tod & & button & m &
 *         Selects the previous ToD. $
 *
 * lawful_bonus & & slider & m &
 *         Sets the Lawful Bonus for the current ToD. $
 *
 * tod_red & & slider & m &
 *         Sets the red component of the current ToD. $
 *
 * tod_green & & slider & m &
 *         Sets the green component of the current ToD. $
 *
 * tod_blue & & slider & m &
 *         Sets the blue component of the current ToD. $
 *
 * @end{table}
 */

REGISTER_DIALOG(custom_tod)

tcustom_tod::tcustom_tod(editor::editor_display* display
		, const std::vector<time_of_day>& tods)
	: tods_(tods)
	, current_tod_(0)
	, current_tod_name_(NULL)
	, current_tod_id_(NULL)
	, current_tod_image_(NULL)
	, current_tod_mask_(NULL)
	, current_tod_sound_(NULL)
	, current_tod_number_(NULL)
	, lawful_bonus_field_(register_integer("lawful_bonus"
				, true))
	, tod_red_field_(register_integer("tod_red"
				, true
				, &preferences::editor::tod_r
				, &preferences::editor::set_tod_r))
	, tod_green_field_(register_integer("tod_green"
				, true
				, &preferences::editor::tod_g
				, &preferences::editor::set_tod_g))
	, tod_blue_field_(register_integer("tod_blue"
				, true
				, &preferences::editor::tod_b
				, &preferences::editor::set_tod_b))
	, display_(display) {}

void tcustom_tod::select_file(const std::string& filename, const std::string& dir, const std::string& vector_attrib, twindow& window)
{
	std::string va = vector_attrib;
	std::string fn = file_name(filename);
    std::string dn = directory_name(fn);
    if(dn.empty()) {
        dn = dir;
    }

    int res = dialogs::show_file_chooser_dialog(*display_, dn, _("Choose file"));
	if (res == 0) {
		if (va == "image") {
			tods_[current_tod_].image = dn; }
		else if (va == "mask") {
			tods_[current_tod_].image_mask = dn; }
		else if (va == "sound") {
			tods_[current_tod_].sounds = dn;
		}
	}
	update_selected_tod_info(window);
}

void tcustom_tod::do_next_tod(twindow& window)
{
	current_tod_ = (current_tod_ + 1) % tods_.size();
	update_selected_tod_info(window);
}

void tcustom_tod::do_prev_tod(twindow& window)
{
	current_tod_ = (current_tod_ ? current_tod_ : tods_.size()) - 1;
	update_selected_tod_info(window);
}

void tcustom_tod::do_new_tod(twindow& window)
{
	tods_.insert(tods_.begin()+current_tod_, time_of_day()); 
	update_selected_tod_info(window);
}

void tcustom_tod::do_delete_tod(twindow& window)
{
	assert(tods_.begin()+current_tod_ < tods_.end());
	if (tods_.size() == 1) {
		tods_.at(0) = time_of_day();
		update_selected_tod_info(window);
		return;
	}
	tods_.erase(tods_.begin()+current_tod_);
	if (tods_.begin()+current_tod_ >= tods_.end()) {
		current_tod_ = 	tods_.size() - 1;
	}
	update_selected_tod_info(window);
}

const std::vector<time_of_day>& tcustom_tod::do_save_schedule() const
{
	return tods_;
}

const time_of_day& tcustom_tod::get_selected_tod() const
{
	assert(static_cast<size_t>(current_tod_) < tods_.size());
	return tods_[current_tod_];
}

void tcustom_tod::update_tod_display(twindow& window)
{
	image::set_color_adjustment(
			  tod_red_field_->get_widget_value(window)
			, tod_green_field_->get_widget_value(window)
			, tod_blue_field_->get_widget_value(window));

	// Prevent a floating slice of window appearing alone over the
	// theme UI sidebar after redrawing tiles and before we have a
	// chance to redraw the rest of this window.
	window.undraw();

	if(display_) {
		// NOTE: We only really want to re-render the gamemap tiles here.
		// Redrawing everything is a significantly more expensive task.
		// At this time, tiles are the only elements on which ToD tint is
		// meant to have a visible effect. This is very strongly tied to
		// the image caching mechanism.
		//
		// If this ceases to be the case in the future, you'll need to call
		// redraw_everything() instead.

		// invalidate all tiles so they are redrawn with the new ToD tint next
		display_->invalidate_all();
		// redraw tiles
		display_->draw();
	}
}

void tcustom_tod::update_lawful_bonus(twindow& window)
{
	tods_[current_tod_].lawful_bonus = lawful_bonus_field_->get_widget_value(window);
}

void tcustom_tod::slider_update_callback(twindow& window)
{
	update_tod_display(window);
}

void tcustom_tod::update_selected_tod_info(twindow& window)
{
	current_tod_name_->set_value(get_selected_tod().name);
	current_tod_id_->set_value(get_selected_tod().id);
	current_tod_image_->set_image(get_selected_tod().image);
	current_tod_mask_->set_image(get_selected_tod().image_mask);
	current_tod_sound_->set_label(get_selected_tod().sounds);
	
	std::stringstream ss;
	ss << (current_tod_ + 1);
	ss << "/" << tods_.size();
	current_tod_number_->set_label(ss.str());

	lawful_bonus_field_->set_widget_value(
			  window
			, get_selected_tod().lawful_bonus);
	tod_red_field_->set_widget_value(
			  window
			, get_selected_tod().color.r);
	tod_green_field_->set_widget_value(
			  window
			, get_selected_tod().color.g);
	tod_blue_field_->set_widget_value(
			  window
			, get_selected_tod().color.b);

    update_tod_display(window);
    window.invalidate_layout();
}

void tcustom_tod::pre_show(CVideo& /*video*/, twindow& window)
{
	assert(!tods_.empty());
	current_tod_name_ = find_widget<ttext_box>(
		&window, "tod_name", false, true);
		
	current_tod_id_ = find_widget<ttext_box>(
		&window, "tod_id", false, true);

	current_tod_image_ = find_widget<timage>(
			&window, "current_tod_image", false, true);
			
	current_tod_mask_ = find_widget<timage>(
			&window, "current_tod_mask", false, true);
			
	current_tod_sound_ = find_widget<tlabel>(
		&window, "current_sound", false, true);

	current_tod_number_ = find_widget<tlabel>(
		&window, "tod_number", false, true);

	tbutton& image_button = find_widget<tbutton>(
			&window, "image_button", false);
	connect_signal_mouse_left_click(image_button, boost::bind(
			  &tcustom_tod::select_file, this, get_selected_tod().image, "data/core/images/misc", "image"
			, boost::ref(window)));

	tbutton& mask_button = find_widget<tbutton>(
			&window, "mask_button", false);;
	connect_signal_mouse_left_click(mask_button, boost::bind(
			  &tcustom_tod::select_file, this, get_selected_tod().image_mask, "data/core/images", "mask"
			, boost::ref(window)));
			
	tbutton& sound_button = find_widget<tbutton>(
			&window, "sound_button", false);
	connect_signal_mouse_left_click(sound_button, boost::bind(
			  &tcustom_tod::select_file, this, get_selected_tod().sounds, "data/core/sounds/ambient", "sound"
			, boost::ref(window)));
			
	tbutton& next_tod_button = find_widget<tbutton>(
			&window, "next_tod", false);
	connect_signal_mouse_left_click(next_tod_button, boost::bind(
			  &tcustom_tod::do_next_tod
			, this
			, boost::ref(window)));

	tbutton& prev_tod_button = find_widget<tbutton>(
			&window, "previous_tod", false);
	connect_signal_mouse_left_click(prev_tod_button, boost::bind(
			  &tcustom_tod::do_prev_tod
			, this
			, boost::ref(window)));

	tbutton& new_button = find_widget<tbutton>(
			&window, "new", false);
	connect_signal_mouse_left_click(new_button, boost::bind(
			  &tcustom_tod::do_new_tod
			, this
			, boost::ref(window)));

	tbutton& delete_button = find_widget<tbutton>(
			&window, "delete", false);
	connect_signal_mouse_left_click(delete_button, boost::bind(
			  &tcustom_tod::do_delete_tod
			, this
			, boost::ref(window)));
			
	tbutton& save_button = find_widget<tbutton>(
			&window, "save", false);
	connect_signal_mouse_left_click(save_button, boost::bind(
			  &tcustom_tod::do_save_schedule
			, this));

	connect_signal_notify_modified(*(lawful_bonus_field_->widget())
			, boost::bind(
				  &tcustom_tod::update_lawful_bonus
				, this
				, boost::ref(window)));
				
	connect_signal_notify_modified(*(tod_red_field_->widget())
			, boost::bind(
				  &tcustom_tod::slider_update_callback
				, this
				, boost::ref(window)));

	connect_signal_notify_modified(*(tod_green_field_->widget())
			, boost::bind(
				  &tcustom_tod::slider_update_callback
				, this
				, boost::ref(window)));

	connect_signal_notify_modified(*(tod_blue_field_->widget())
			, boost::bind(
				  &tcustom_tod::slider_update_callback
				, this
				, boost::ref(window)));

	for (size_t i = 0; i < tods_.size(); ++i) {

		time_of_day& tod = tods_[i];
		const int r = tod_red_field_->get_widget_value(window);
		const int g = tod_green_field_->get_widget_value(window);
		const int b = tod_blue_field_->get_widget_value(window);
		if (tod.color.r == r && tod.color.g == g && tod.color.b == b) {
			current_tod_ = i;
			update_selected_tod_info(window);
			return;
		}
	}

	update_selected_tod_info(window);
}

void tcustom_tod::post_show(twindow& window)
{
    update_tod_display(window);
}

} // namespace gui2
