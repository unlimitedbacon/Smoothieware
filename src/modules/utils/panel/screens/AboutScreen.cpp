/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Panel.h"
#include "PanelScreen.h"
#include "LcdBase.h"
#include "AboutScreen.h"
#include "PublicData.h"
#include "modules/utils/about/AboutPublicAccess.h"
#include "checksumm.h"
#include "version.h"

#include <string>
using namespace std;

AboutScreen::AboutScreen()
{
}

void AboutScreen::on_enter()
{
    // Get information
    Version v;
    string build = v.get_build();
    string date = v.get_build_date();
    
    struct pad_about a;
    PublicData::get_value(about_checksum, &a);

    int i = 0;
    if (!a.make.empty() or !a.model.empty() or !a.machine_name.empty()) {
        text[i] = "Machine:"; i++;
        if (!a.machine_name.empty()) {
            text[i] = " " + a.machine_name; i++;
        }
        if (!a.make.empty() or !a.model.empty()) {
            text[i] = " " + a.make + " " + a.model; i++;
        }
        text[i] = ""; i++;
    }
    text[i] = "Smoothieware:"; i++;
    text[i] = " " + build; i++;
    text[i] = " " + date.substr(0,17); i++;

    THEPANEL->enter_menu_mode();
    THEPANEL->setup_menu(i);
    this->refresh_menu();
}

void AboutScreen::on_refresh()
{
    if ( THEPANEL->menu_change() ) {
        this->refresh_menu();
    }
    if ( THEPANEL->click() ) {
        this->clicked_menu_entry(THEPANEL->get_menu_current_line());
    }
}

void AboutScreen::display_menu_line(uint16_t line)
{
    THEPANEL->lcd->printf("%s", text[line].c_str());
}

void AboutScreen::clicked_menu_entry(uint16_t line)
{
    THEPANEL->enter_screen(this->parent);
}

