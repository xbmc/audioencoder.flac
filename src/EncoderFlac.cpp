/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
#include "kodi/xbmc_audioenc_dll.h"
#include <string.h>

extern "C" {

static const int SAMPLES_BUF_SIZE = 1024 * 2;

// structure for holding our context
class flac_context
{
public:
  flac_context(FLAC__StreamEncoder *enc, audioenc_callbacks &cb) :
    callbacks(cb),
    tellPos(0),
    encoder(enc)
  {
    metadata[0] = NULL;
    metadata[1] = NULL;
  };

  audioenc_callbacks    callbacks;
  int64_t               tellPos; ///< position for tell() callback
  FLAC__StreamEncoder*  encoder;
  FLAC__StreamMetadata* metadata[2];
  FLAC__int32           samplesBuf[SAMPLES_BUF_SIZE];
};

// settings (currently global)
int level=4;


FLAC__StreamEncoderWriteStatus write_callback_flac(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
  flac_context *context = (flac_context *)client_data;
  if (context && context->callbacks.write)
  {
    if (context->callbacks.write(context->callbacks.opaque, (uint8_t*)buffer, bytes) == bytes)
    {
      context->tellPos += bytes;
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
  }
  return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

FLAC__StreamEncoderSeekStatus seek_callback_flac(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
  flac_context *context = (flac_context *)client_data;
  if (context && context->callbacks.seek)
  {
    if (context->callbacks.seek(context->callbacks.opaque, (int64_t)absolute_byte_offset, 0) == absolute_byte_offset)
    {
      context->tellPos = absolute_byte_offset;
      return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }
  }
  return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
}

FLAC__StreamEncoderTellStatus tell_callback_flac(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
  // libFLAC will cope without a real tell callback
  flac_context *context = (flac_context *)client_data;
  if (context)
  {
    *absolute_byte_offset = context->tellPos;
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
  }
  return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  if (strcmp(strSetting,"level") == 0)
    level = *((int*)value);
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

void* Create(audioenc_callbacks *callbacks)
{
  if (callbacks && callbacks->write && callbacks->seek)
  {
    // allocate libFLAC encoder
    FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
    if (!encoder)
      return NULL;

    return new flac_context(encoder, *callbacks);
  }
  return NULL;
}

bool Start(void *ctx, int iInChannels, int iInRate, int iInBits,
           const char* title, const char* artist,
           const char* albumartist, const char* album,
           const char* year, const char* track, const char* genre,
           const char* comment, int iTrackLength)
{
  flac_context *context = (flac_context *)ctx;
  if (!context || !context->encoder)
    return false;

  // we accept only 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16)
    return false;

  FLAC__bool ok = 1;

  ok &= FLAC__stream_encoder_set_verify(context->encoder, true);
  ok &= FLAC__stream_encoder_set_channels(context->encoder, iInChannels);
  ok &= FLAC__stream_encoder_set_bits_per_sample(context->encoder, iInBits);
  ok &= FLAC__stream_encoder_set_sample_rate(context->encoder, iInRate);
  ok &= FLAC__stream_encoder_set_total_samples_estimate(context->encoder, (FLAC__uint64)iTrackLength * iInRate);
  ok &= FLAC__stream_encoder_set_compression_level(context->encoder, level);

  // now add some metadata
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  if (ok)
  {
    if (
      (context->metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
      (context->metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", artist) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", album) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUMARTIST", albumartist) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", title) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "GENRE", genre) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", track) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "DATE", year) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "COMMENT", comment) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(context->metadata[0], entry, false)
      )
    {
      ok = false;
    }
    else
    {
      context->metadata[1]->length = 4096;
      ok = FLAC__stream_encoder_set_metadata(context->encoder, context->metadata, 2);
    }
  }

  // initialize encoder in stream mode
  if (ok)
  {
    FLAC__StreamEncoderInitStatus init_status;
    init_status = FLAC__stream_encoder_init_stream(context->encoder, write_callback_flac, seek_callback_flac, tell_callback_flac, NULL, context);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      ok = false;
    }
  }

  if (!ok)
  {
    return false;
  }

  return true;
}

int Encode(void *ctx, int nNumBytesRead, uint8_t* pbtStream)
{
  flac_context *context = (flac_context*)ctx;
  if (!context || !context->encoder)
    return 0;

  int nLeftSamples = nNumBytesRead / 2; // each sample takes 2 bytes (16 bits per sample)
  while (nLeftSamples > 0)
  {
    int nSamples = nLeftSamples > SAMPLES_BUF_SIZE ? SAMPLES_BUF_SIZE : nLeftSamples;

    // convert the packed little-endian 16-bit PCM samples into an interleaved FLAC__int32 buffer for libFLAC
    for (int i = 0; i < nSamples; i++)
    { // inefficient but simple and works on big- or little-endian machines.
      context->samplesBuf[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)pbtStream[2*i+1] << 8) | (FLAC__int16)pbtStream[2*i]);
    }

    // feed samples to encoder
    if (!FLAC__stream_encoder_process_interleaved(context->encoder, context->samplesBuf, nSamples / 2))
    {
      return 0;
    }

    nLeftSamples -= nSamples;
    pbtStream += nSamples * 2; // skip processed samples
  }
  return nNumBytesRead; // consumed everything
}

bool Finish(void *ctx)
{
  flac_context *context = (flac_context*)ctx;
  if (!context || !context->encoder)
    return false;

  FLAC__stream_encoder_finish(context->encoder);
  return true;
}

void Free(void *ctx)
{
  flac_context *context = (flac_context*)ctx;
  if (context)
  {
    // free the metadata
    if (context->metadata[0])
      FLAC__metadata_object_delete(context->metadata[0]);
    if (context->metadata[1])
      FLAC__metadata_object_delete(context->metadata[1]);

    // free the encoder
    if (context->encoder)
      FLAC__stream_encoder_delete(context->encoder);

    delete context;
  }
}

}
