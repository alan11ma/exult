/*
Copyright (C) 2000  Dancer A.L Vesperman

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#if __GNUG__ >= 2
#  pragma implementation
#endif


#include "Midi.h"

#include <unistd.h>
#include <csignal>
#include "../fnames.h"

#include "Configuration.h"
extern	Configuration	config;


void    MyMidiPlayer::start_track(int num,int repeats)
{
#if DEBUG
        cout << "Audio subsystem request: Music track # " << num << endl;
#endif
        uint32  length;
        char    *music=midi_tracks.read_object(num,length);
        if(!music)
                return;
        FILE    *fp;
        unlink("/tmp/u7midi");
        fp=fopen("/tmp/u7midi","wb");
        if(!fp)
                {
                delete [] music;
                return;
                }
        fwrite(music,length,1,fp);
        fclose(fp);
	if(!midi_device)
		return;
	midi_device->start_track("/tmp/u7midi",repeats);
}

void	MyMidiPlayer::start_music(int num,int repeats)
{
	if(!midi_device)
		return;
	if(current_track==num&&midi_device->is_playing())
		return;	// Already playing it
	current_track=num;
	start_track(num);
}


#include "midi_drivers/Timidity_binary.h"
#include "midi_drivers/KMIDI.h"
#include "midi_drivers/forked_player.h"

MyMidiPlayer::MyMidiPlayer()	: current_track(-1),midi_device(0)
{
	bool	no_device=true;
	midi_tracks=AccessFlexFile(ADLIBMUS);
	instrument_patches=AccessTableFile(XMIDI_MT);
#if DEBUG
	cerr << "Read in " << midi_tracks.object_list.size() << " tracks" << endl;
#endif
	string	s;
	config.value("config/audio/midi/enabled",s,"---");
	if(s=="---")
		{
		cout << "Config does not specify MIDI. Assuming yes" << endl;
		s="yes";
		}
	if(s=="no")
		{
		cout << "Config says no midi. MIDI disabled";
		no_device=false;
		}
	config.set("config/audio/midi/enabled",s,true);

	if(no_device)
		{
		try {
#if HAVE_TIMIDITY_BIN
		midi_device=new Timidity_binary();
#else
		throw 0;
#endif
		no_device=false;
		cerr << midi_device->copyright() << endl;
		} catch(...)
			{
			no_device=true;
			}
		}
	if(no_device)
		{
		try {
#if HAVE_LIBKMIDI
		midi_device=new KMIDI();
#else
		throw 0;
#endif
		no_device=false;
		cerr << midi_device->copyright() << endl;
		} catch(...)
			{
			no_device=true;
			}
		}


	if(no_device)
		{
		try {
			midi_device=new forked_player();
			no_device=false;
			cerr << midi_device->copyright() << endl;
			} catch(...)
			{
			no_device=true;
			}
		}
	
	if(no_device)
		{
		midi_device=0;
		cerr << "Unable to create a music device. No music will be played" << endl;
		}
}

MyMidiPlayer::~MyMidiPlayer()
{
	if(midi_device&&midi_device->is_playing())
		midi_device->stop_track();
}

