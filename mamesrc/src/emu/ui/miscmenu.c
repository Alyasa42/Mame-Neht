/*********************************************************************

    miscmenu.c

    Internal MAME menus for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "emu.h"
#include "osdnet.h"

#include "uiinput.h"
#include "ui/ui.h"
#include "ui/miscmenu.h"
#include "ui/filemngr.h"
#include "rendfont.h" // For convert_command_glyph
#ifdef CMD_LIST
#include "cmddata.h"
#endif /* CMD_LIST */
#ifdef USE_SCALE_EFFECTS
#include "osdscale.h"
#endif /* USE_SCALE_EFFECTS */


/***************************************************************************
    MENU HANDLERS
***************************************************************************/

/*-------------------------------------------------
    ui_menu_keyboard_mode - menu that
-------------------------------------------------*/

ui_menu_keyboard_mode::ui_menu_keyboard_mode(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

void ui_menu_keyboard_mode::populate()
{
	bool natural = machine().ui().use_natural_keyboard();
	item_append(_("Keyboard Mode:"), natural ? _("Natural") : _("Emulated"), natural ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, NULL);
}

ui_menu_keyboard_mode::~ui_menu_keyboard_mode()
{
}

void ui_menu_keyboard_mode::handle()
{
	bool natural = machine().ui().use_natural_keyboard();

	/* process the menu */
	const ui_menu_event *menu_event = process(0);

	if (menu_event != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
		{
			machine().ui().set_use_natural_keyboard(natural ^ true);
			reset(UI_MENU_RESET_REMEMBER_REF);
		}
	}
}


/*-------------------------------------------------
    ui_menu_bios_selection - populates the main
    bios selection menu
-------------------------------------------------*/

ui_menu_bios_selection::ui_menu_bios_selection(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

void ui_menu_bios_selection::populate()
{
	/* cycle through all devices for this system */
	device_iterator deviter(machine().root_device());
	for (device_t *device = deviter.first(); device != NULL; device = deviter.next())
	{
		if (device->rom_region()) {
			const char *val = "default";
			for (const rom_entry *rom = device->rom_region(); !ROMENTRY_ISEND(rom); rom++)
			{
				if (ROMENTRY_ISSYSTEM_BIOS(rom) && ROM_GETBIOSFLAGS(rom)==device->system_bios())
				{
					val = ROM_GETHASHDATA(rom);
				}
			}
			item_append(strcmp(device->tag(),":")==0 ? _("driver") : device->tag()+1, val, MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)device);
		}
	}

	item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);
	item_append(_("Reset"),  NULL, 0, (void *)1);
}

ui_menu_bios_selection::~ui_menu_bios_selection()
{
}

/*-------------------------------------------------
    ui_menu_bios_selection - menu that
-------------------------------------------------*/

void ui_menu_bios_selection::handle()
{
	/* process the menu */
	const ui_menu_event *menu_event = process(0);

	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		if ((FPTR)menu_event->itemref == 1 && menu_event->iptkey == IPT_UI_SELECT)
			machine().schedule_hard_reset();
		else if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
		{
			device_t *dev = (device_t *)menu_event->itemref;
			int cnt = 0;
			for (const rom_entry *rom = dev->rom_region(); !ROMENTRY_ISEND(rom); rom++)
			{
				if (ROMENTRY_ISSYSTEM_BIOS(rom)) cnt ++;
			}
			int val = dev->system_bios() + ((menu_event->iptkey == IPT_UI_LEFT) ? -1 : +1);
			if (val<1) val=cnt;
			if (val>cnt) val=1;
			dev->set_system_bios(val);
			if (strcmp(dev->tag(),":")==0) {
				astring error;
				machine().options().set_value("bios", val-1, OPTION_PRIORITY_CMDLINE, error);
				assert(!error);
			} else {
				astring error;
				astring value;
				astring temp;
				value.printf("%s,bios=%d",machine().options().main_value(temp,dev->owner()->tag()+1),val-1);
				machine().options().set_value(dev->owner()->tag()+1, value.cstr(), OPTION_PRIORITY_CMDLINE, error);
				assert(!error);
			}
			reset(UI_MENU_RESET_REMEMBER_REF);
		}
	}
}



ui_menu_network_devices::ui_menu_network_devices(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_network_devices::~ui_menu_network_devices()
{
}

/*-------------------------------------------------
    menu_network_devices_populate - populates the main
    network device menu
-------------------------------------------------*/

void ui_menu_network_devices::populate()
{
	/* cycle through all devices for this system */
	network_interface_iterator iter(machine().root_device());
	for (device_network_interface *network = iter.first(); network != NULL; network = iter.next())
	{
		int curr = network->get_interface();
		const char *title = NULL;
		const osd_netdev::entry_t *entry = netdev_first();
		while(entry) {
			if(entry->id==curr) {
				title = entry->description;
				break;
			}
			entry = entry->m_next;
		}

		item_append(network->device().tag(),  (title) ? title : "------", MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)network);
	}
}

/*-------------------------------------------------
    ui_menu_network_devices - menu that
-------------------------------------------------*/

void ui_menu_network_devices::handle()
{
	/* process the menu */
	const ui_menu_event *menu_event = process(0);

	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT) {
			device_network_interface *network = (device_network_interface *)menu_event->itemref;
			int curr = network->get_interface();
			if (menu_event->iptkey == IPT_UI_LEFT) curr--; else curr++;
			if (curr==-2) curr = netdev_count() - 1;
			network->set_interface(curr);
			reset(UI_MENU_RESET_REMEMBER_REF);
		}
	}
}


/*-------------------------------------------------
    menu_bookkeeping - handle the bookkeeping
    information menu
-------------------------------------------------*/

void ui_menu_bookkeeping::handle()
{
	attotime curtime;

	/* if the time has rolled over another second, regenerate */
	curtime = machine().time();
	if (prevtime.seconds != curtime.seconds)
	{
		reset(UI_MENU_RESET_SELECT_FIRST);
		prevtime = curtime;
		populate();
	}

	/* process the menu */
	process(0);
}


/*-------------------------------------------------
    menu_bookkeeping - handle the bookkeeping
    information menu
-------------------------------------------------*/
ui_menu_bookkeeping::ui_menu_bookkeeping(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_bookkeeping::~ui_menu_bookkeeping()
{
}

void ui_menu_bookkeeping::populate()
{
	int tickets = get_dispensed_tickets(machine());
	astring tempstring;
	int ctrnum;

	/* show total time first */
	if (prevtime.seconds >= 60 * 60)
		tempstring.catprintf(_("Uptime: %d:%02d:%02d\n\n"), prevtime.seconds / (60*60), (prevtime.seconds / 60) % 60, prevtime.seconds % 60);
	else
		tempstring.catprintf(_("Uptime: %d:%02d\n\n"), (prevtime.seconds / 60) % 60, prevtime.seconds % 60);

	/* show tickets at the top */
	if (tickets > 0)
		tempstring.catprintf(_("Tickets dispensed: %d\n\n"), tickets);

	/* loop over coin counters */
	for (ctrnum = 0; ctrnum < COIN_COUNTERS; ctrnum++)
	{
		int count = coin_counter_get_count(machine(), ctrnum);

		/* display the coin counter number */
		tempstring.catprintf(_("Coin %c: "), ctrnum + 'A');

		/* display how many coins */
		if (count == 0)
			tempstring.cat(_("NA"));
		else
			tempstring.catprintf("%d", count);

		/* display whether or not we are locked out */
		if (coin_lockout_get_state(machine(), ctrnum))
			tempstring.cat(_(" (locked)"));
		tempstring.cat("\n");
	}

	/* append the single item */
	item_append(tempstring, NULL, MENU_FLAG_MULTILINE, NULL);
}

/*-------------------------------------------------
    menu_crosshair - handle the crosshair settings
    menu
-------------------------------------------------*/

void ui_menu_crosshair::handle()
{
	/* process the menu */
	const ui_menu_event *menu_event = process(UI_MENU_PROCESS_LR_REPEAT);

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		crosshair_user_settings settings;
		crosshair_item_data *data = (crosshair_item_data *)menu_event->itemref;
		bool changed = false;
		//int set_def = false;
		int newval = data->cur;

		/* retreive the user settings */
		crosshair_get_user_settings(machine(), data->player, &settings);

		switch (menu_event->iptkey)
		{
			/* if selected, reset to default value */
			case IPT_UI_SELECT:
				newval = data->defvalue;
				//set_def = true;
				break;

			/* left decrements */
			case IPT_UI_LEFT:
				newval -= machine().input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;

			/* right increments */
			case IPT_UI_RIGHT:
				newval += machine().input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;
		}

		/* clamp to range */
		if (newval < data->min)
			newval = data->min;
		if (newval > data->max)
			newval = data->max;

		/* if things changed, update */
		if (newval != data->cur)
		{
			switch (data->type)
			{
				/* visibility state */
				case CROSSHAIR_ITEM_VIS:
					settings.mode = newval;
					changed = true;
					break;

				/* auto time */
				case CROSSHAIR_ITEM_AUTO_TIME:
					settings.auto_time = newval;
					changed = true;
					break;
			}
		}

		/* crosshair graphic name */
		if (data->type == CROSSHAIR_ITEM_PIC)
		{
			switch (menu_event->iptkey)
			{
				case IPT_UI_SELECT:
					/* clear the name string to reset to default crosshair */
					settings.name[0] = 0;
					changed = true;
					break;

				case IPT_UI_LEFT:
					strcpy(settings.name, data->last_name);
					changed = true;
					break;

				case IPT_UI_RIGHT:
					strcpy(settings.name, data->next_name);
					changed = true;
					break;
			}
		}

		if (changed)
		{
			/* save the user settings */
			crosshair_set_user_settings(machine(), data->player, &settings);

			/* rebuild the menu */
			reset(UI_MENU_RESET_REMEMBER_POSITION);
		}
	}
}


/*-------------------------------------------------
    menu_crosshair_populate - populate the
    crosshair settings menu
-------------------------------------------------*/

ui_menu_crosshair::ui_menu_crosshair(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

void ui_menu_crosshair::populate()
{
	crosshair_user_settings settings;
	crosshair_item_data *data;
	char temp_text[16];
	int player;
	UINT8 use_auto = false;
	UINT32 flags = 0;

	/* loop over player and add the manual items */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		/* get the user settings */
		crosshair_get_user_settings(machine(), player, &settings);

		/* add menu items for usable crosshairs */
		if (settings.used)
		{
			/* Make sure to keep these matched to the CROSSHAIR_VISIBILITY_xxx types */
			static const char *const vis_text[] = { "Off", "On", "Auto" };

			/* track if we need the auto time menu */
			if (settings.mode == CROSSHAIR_VISIBILITY_AUTO) use_auto = true;

			/* CROSSHAIR_ITEM_VIS - allocate a data item and fill it */
			data = (crosshair_item_data *)m_pool_alloc(sizeof(*data));
			data->type = CROSSHAIR_ITEM_VIS;
			data->player = player;
			data->min = CROSSHAIR_VISIBILITY_OFF;
			data->max = CROSSHAIR_VISIBILITY_AUTO;
			data->defvalue = CROSSHAIR_VISIBILITY_DEFAULT;
			data->cur = settings.mode;

			/* put on arrows */
			if (data->cur > data->min)
				flags |= MENU_FLAG_LEFT_ARROW;
			if (data->cur < data->max)
				flags |= MENU_FLAG_RIGHT_ARROW;

			/* add CROSSHAIR_ITEM_VIS menu */
			sprintf(temp_text, _("P%d Visibility"), player + 1);
			item_append(temp_text, _(vis_text[settings.mode]), flags, data);

			/* CROSSHAIR_ITEM_PIC - allocate a data item and fill it */
			data = (crosshair_item_data *)m_pool_alloc(sizeof(*data));
			data->type = CROSSHAIR_ITEM_PIC;
			data->player = player;
			data->last_name[0] = 0;
			/* other data item not used by this menu */

			/* search for crosshair graphics */

			/* open a path to the crosshairs */
			file_enumerator path(machine().options().crosshair_path());
			const osd_directory_entry *dir;
			/* reset search flags */
			int using_default = false;
			int finished = false;
			int found = false;

			/* if we are using the default, then we just need to find the first in the list */
			if (*(settings.name) == 0)
				using_default = true;

			/* look for the current name, then remember the name before */
			/* and find the next name */
			while (((dir = path.next()) != NULL) && !finished)
			{
				int length = strlen(dir->name);

				/* look for files ending in .png with a name not larger then 9 chars*/
				if ((length > 4) && (length <= CROSSHAIR_PIC_NAME_LENGTH + 4) &&
					dir->name[length - 4] == '.' &&
					tolower((UINT8)dir->name[length - 3]) == 'p' &&
					tolower((UINT8)dir->name[length - 2]) == 'n' &&
					tolower((UINT8)dir->name[length - 1]) == 'g')

				{
					/* remove .png from length */
					length -= 4;

					if (found || using_default)
					{
						/* get the next name */
						strncpy(data->next_name, dir->name, length);
						data->next_name[length] = 0;
						finished = true;
					}
					else if (!strncmp(dir->name, settings.name, length))
					{
						/* we found the current name */
						/* so loop once more to find the next name */
						found = true;
					}
					else
						/* remember last name */
						/* we will do it here in case files get added to the directory */
					{
						strncpy(data->last_name, dir->name, length);
						data->last_name[length] = 0;
					}
				}
			}
			/* if name not found then next item is DEFAULT */
			if (!found && !using_default)
			{
				data->next_name[0] = 0;
				finished = true;
			}
			/* setup the selection flags */
			flags = 0;
			if (finished)
				flags |= MENU_FLAG_RIGHT_ARROW;
			if (found)
				flags |= MENU_FLAG_LEFT_ARROW;

			/* add CROSSHAIR_ITEM_PIC menu */
			sprintf(temp_text, _("P%d Crosshair"), player + 1);
			item_append(temp_text, using_default ? _("DEFAULT") : settings.name, flags, data);
		}
	}
	if (use_auto)
	{
		/* any player can be used to get the autotime */
		crosshair_get_user_settings(machine(), 0, &settings);

		/* CROSSHAIR_ITEM_AUTO_TIME - allocate a data item and fill it */
		data = (crosshair_item_data *)m_pool_alloc(sizeof(*data));
		data->type = CROSSHAIR_ITEM_AUTO_TIME;
		data->min = CROSSHAIR_VISIBILITY_AUTOTIME_MIN;
		data->max = CROSSHAIR_VISIBILITY_AUTOTIME_MAX;
		data->defvalue = CROSSHAIR_VISIBILITY_AUTOTIME_DEFAULT;
		data->cur = settings.auto_time;

		/* put on arrows in visible menu */
		if (data->cur > data->min)
			flags |= MENU_FLAG_LEFT_ARROW;
		if (data->cur < data->max)
			flags |= MENU_FLAG_RIGHT_ARROW;

		/* add CROSSHAIR_ITEM_AUTO_TIME menu */
		sprintf(temp_text, "%d", settings.auto_time);
		item_append(_("Visible Delay"), temp_text, flags, data);
	}
//  else
//      /* leave a blank filler line when not in auto time so size does not rescale */
//      item_append("", "", NULL, NULL);
}

ui_menu_crosshair::~ui_menu_crosshair()
{
}

#ifdef USE_SCALE_EFFECTS
#define SCALE_ITEM_NONE 0
/*-------------------------------------------------
    menu_scale_effect - handle the scale effect
    settings menu
-------------------------------------------------*/

ui_menu_scale_effect::ui_menu_scale_effect(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_scale_effect::~ui_menu_scale_effect()
{
}

void ui_menu_scale_effect::handle()
{
	const ui_menu_event *menu_event = process(0);
	bool changed = false;

	/* process the menu */
	if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT && 
		(FPTR)menu_event->itemref >= SCALE_ITEM_NONE)
	{
		screen_device *screen = machine().first_screen();
		screen->video_exit_scale_effect();
		scale_decode(scale_name((FPTR)menu_event->itemref - SCALE_ITEM_NONE));
		screen->video_init_scale_effect();
		changed = true;
		osd_printf_verbose(_("scale effect: %s\n"), scale_name((FPTR)menu_event->itemref - SCALE_ITEM_NONE));
	}

	/* if something changed, rebuild the menu */
	if (changed)
		reset(UI_MENU_RESET_REMEMBER_REF);
}


/*-------------------------------------------------
    menu_scale_effect_populate - populate the
    scale effect menu
-------------------------------------------------*/

void ui_menu_scale_effect::populate()
{
	int scaler;
	item_append(_("None"), NULL, 0, (void *)SCALE_ITEM_NONE);

	/* add items for each scaler */
	for (scaler = 1; ; scaler++)
	{
		const char *desc = scale_desc(scaler);
		if (desc == NULL)
			break;

		/* create a string for the item */
		item_append(desc, NULL, 0, (void *)(SCALE_ITEM_NONE + scaler));
	}
	selected = scale_effect.effect;
}
#undef SCALE_ITEM_NONE
#endif /* USE_SCALE_EFFECTS */


#ifdef USE_AUTOFIRE
#define AUTOFIRE_ITEM_P1_DELAY 1
/*-------------------------------------------------
    menu_autofire - handle the autofire settings
    menu
-------------------------------------------------*/

ui_menu_autofire::ui_menu_autofire(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_autofire::~ui_menu_autofire()
{
}

void ui_menu_autofire::handle()
{
	bool changed = false;

	/* process the menu */
	const ui_menu_event *menu_event = process(0);
	
	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
		{
			int player = (int)(FPTR)menu_event->itemref - AUTOFIRE_ITEM_P1_DELAY;
			//autofire delay
			if (player >= 0 && player < MAX_PLAYERS)
			{
				int autofire_delay = machine().ioport().get_autofiredelay(player);

				if (menu_event->iptkey == IPT_UI_LEFT)
				{
					autofire_delay--;
					if (autofire_delay < 1)
						autofire_delay = 1;
				}
				else
				{
					autofire_delay++;
					if (autofire_delay > 99)
						autofire_delay = 99;
				}

				machine().ioport().set_autofiredelay(player, autofire_delay);

				changed = true;
			}
			//anything else is a toggle item
			else
			{
				ioport_field *field = (ioport_field *)menu_event->itemref;
				ioport_field::user_settings settings;
				int selected_value;
				field->get_user_settings(settings);
				selected_value = settings.autofire;

				if (menu_event->iptkey == IPT_UI_LEFT)
				{
					if (--selected_value < 0)
					selected_value = 2;
				}
				else
				{
					if (++selected_value > 2)
					selected_value = 0;	
				}

				settings.autofire = selected_value;
				field->set_user_settings(settings);

				changed = true;
			}
		}
	}

	/* if something changed, rebuild the menu */
	if (changed)
		reset(UI_MENU_RESET_REMEMBER_REF);
}


/*-------------------------------------------------
    menu_autofire_populate - populate the autofire
    menu
-------------------------------------------------*/

void ui_menu_autofire::populate()
{
	astring subtext;
	astring text;
	ioport_field *field;
	ioport_port *port;
	int players = 0;
	int i;

	/* iterate over the input ports and add autofire toggle items */
	for (port = machine().ioport().first_port(); port != NULL; port = port->next())
		for (field = port->first_field(); field != NULL; field = field->next())
		{
			const char *name = field->name();

			if (name != NULL && (
			    (field->type() >= IPT_BUTTON1 && field->type() < IPT_BUTTON1 + MAX_NORMAL_BUTTONS)
#ifdef USE_CUSTOM_BUTTON
			    || (field->type() >= IPT_CUSTOM1 && field->type() < IPT_CUSTOM1 + MAX_CUSTOM_BUTTONS)
#endif /* USE_CUSTOM_BUTTON */
			   ))
			{
				ioport_field::user_settings settings;
				field->get_user_settings(settings);
//				entry[menu_items] = field;

				if (players < field->player() + 1)
					players = field->player() + 1;

				/* add an autofire item */
				switch (settings.autofire)
				{
					case 0:	subtext.cpy(_("Off"));	break;
					case 1:	subtext.cpy(_("On"));		break;
					case 2:	subtext.cpy(_("Toggle"));	break;
				}
				item_append(_(field->name()), subtext, MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)field);
			}
		}
	
	/* add autofire delay items */
	for (i = 0; i < players; i++)
	{
		text.printf(_("P%d %s"), i + 1, _("Autofire Delay"));
		subtext.printf("%d", machine().ioport().get_autofiredelay(i));

		/* append a menu item */
		item_append(text, subtext, MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)(i + AUTOFIRE_ITEM_P1_DELAY));
	}
}
#undef AUTOFIRE_ITEM_P1_DELAY
#endif /* USE_AUTOFIRE */


#ifdef USE_CUSTOM_BUTTON
/*-------------------------------------------------
    menu_custom_button - handle the custom button
    settings menu
-------------------------------------------------*/

ui_menu_custom_button::ui_menu_custom_button(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_custom_button::~ui_menu_custom_button()
{
}

void ui_menu_custom_button::handle()
{
	const ui_menu_event *menu_event = process(0);
	bool changed = false;
	int custom_buttons_count = 0;
	ioport_field *field;
	ioport_port *port;

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		UINT16 *selected_custom_button = (UINT16 *)(FPTR)menu_event->itemref;
		int i;
		
		//count the number of custom buttons
		for (port = machine().ioport().first_port(); port != NULL; port = port->next())
			for (field = port->first_field(); field != NULL; field = field->next())
			{
				int type = field->type();

				if (type >= IPT_BUTTON1 && type < IPT_BUTTON1 + MAX_NORMAL_BUTTONS)
				{
					type -= IPT_BUTTON1;
					if (type >= custom_buttons_count)
						custom_buttons_count = type + 1;
				}
			}

		input_item_id id = ITEM_ID_1;
		for (i = 0; i < custom_buttons_count; i++, id++)
		{
			if (i == 9)
				id = ITEM_ID_0;

			//fixme: code_pressed_once() doesn't work well
			if (machine().input().code_pressed_once(input_code(DEVICE_CLASS_KEYBOARD, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, id)))
			{
				*selected_custom_button ^= 1 << i;
				changed = true;
				break;
			}
		}
	}

	/* if something changed, rebuild the menu */
	if (changed)
		reset(UI_MENU_RESET_REMEMBER_REF);
}


/*-------------------------------------------------
    menu_custom_button_populate - populate the 
    custom button menu
-------------------------------------------------*/

void ui_menu_custom_button::populate()
{
	astring subtext;
	astring text;
	ioport_field *field;
	ioport_port *port;
	int menu_items = 0;
	int is_neogeo = !core_stricmp(machine().system().source_file+17, "neogeo.c")
					|| !core_stricmp(machine().system().source_file+17, "neogeo_noslot.c");
	int i;

//	item_append(_("Press 1-9 to Config"), NULL, 0, NULL);
//	item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);

	/* loop over the input ports and add autofire toggle items */
	for (port = machine().ioport().first_port(); port != NULL; port = port->next())
		for (field = port->first_field(); field != NULL; field = field->next())
		{
			int player = field->player();
			int type = field->type();
			const char *name = field->name();

			if (name != NULL && type >= IPT_CUSTOM1 && type < IPT_CUSTOM1 + MAX_CUSTOM_BUTTONS)
			{
				const char colorbutton1 = is_neogeo ? 'A' : 'a';
				int n = 1;
				static char commandbuf[256];

				type -= IPT_CUSTOM1;
				subtext.cpy("");

				//unpack the custom button value
				for (i = 0; i < MAX_NORMAL_BUTTONS; i++, n <<= 1)
					if (machine().ioport().m_custom_button[player][type] & n)
					{
						if (subtext.len() > 0)
							subtext.cat("_+");
						subtext.catprintf("_%c", colorbutton1 + i);
					}

				strcpy(commandbuf, subtext);
				convert_command_glyph(commandbuf, ARRAY_LENGTH(commandbuf));
				item_append(_(name), commandbuf, 0, (void *)(FPTR)&machine().ioport().m_custom_button[player][type]);

				menu_items++;
			}
		}
}
#endif /* USE_CUSTOM_BUTTON */

#ifdef CMD_LIST
/*-------------------------------------------------
    menu_command - handle the command.dat
    menu
-------------------------------------------------*/

ui_menu_command::ui_menu_command(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_command::~ui_menu_command()
{
}

void ui_menu_command::populate()
{
	const char *item[256];
	int menu_items;
	int total = command_sub_menu(&machine().system(), item);
		
	if (total)
	{
		for (menu_items = 0; menu_items < total; menu_items++)
			item_append(item[menu_items], NULL, 0, (void *)menu_items);
	}


}

void ui_menu_command::handle()
{
	/* process the menu */
	const ui_menu_event *menu_event = process(0);
	if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT)
		ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_command_content(machine(), container, int((long long)(menu_event->itemref)))));
}

ui_menu_command_content::ui_menu_command_content(running_machine &machine, render_container *container, int _param) : ui_menu_command(machine, container)
{
	param = _param;
}

ui_menu_command_content::~ui_menu_command_content()
{
}

void ui_menu_command_content::handle()
{
	/* process the menu */
	process(UI_MENU_PROCESS_CUSTOM_ONLY);
}

void ui_menu_command_content::populate()
{
	char commandbuf[64 * 1024]; // 64KB of command.dat buffer, enough for everything

	int game_paused = machine().paused();

	/* Disable sound to prevent strange sound*/
	if (!game_paused)
		machine().pause();

	if (load_driver_command_ex(&machine().system(), commandbuf, ARRAY_LENGTH(commandbuf), (FPTR)param) == 0)
	{
		const game_driver *last_drv;
		last_drv = &machine().system();
		convert_command_glyph(commandbuf, ARRAY_LENGTH(commandbuf));

//		item_append(commandbuf, NULL, MENU_FLAG_MULTILINE, NULL);
		machine().ui().draw_message_window_fixed_width(container, commandbuf);
	}

	if (!game_paused)
		machine().resume();

	if (machine().ui().window_scroll_keys() > 0)
		ui_menu::stack_pop(machine());
}

void ui_menu_command_content::custom_render(void *selectedref, float top, float bottom, float x, float y, float x2, float y2)
{
}

#endif /* CMD_LIST */

/*-------------------------------------------------
    menu_quit_game - handle the "menu" for
    quitting the game
-------------------------------------------------*/

ui_menu_quit_game::ui_menu_quit_game(running_machine &machine, render_container *container) : ui_menu(machine, container)
{
}

ui_menu_quit_game::~ui_menu_quit_game()
{
}

void ui_menu_quit_game::populate()
{
}

void ui_menu_quit_game::handle()
{
	/* request a reset */
	machine().schedule_exit();

	/* reset the menu stack */
	ui_menu::stack_reset(machine());
}
