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
#include <kodi/addon-instance/AudioEncoder.h>
#include <string.h>

static const int SAMPLES_BUF_SIZE = 1024 * 2;

class ATTRIBUTE_HIDDEN CEncoderFlac : public kodi::addon::CInstanceAudioEncoder
{
public:
  CEncoderFlac(KODI_HANDLE instance);
  ~CEncoderFlac() override;

  bool Start(int inChannels,
             int inRate,
             int inBits,
             const std::string& title,
             const std::string& artist,
             const std::string& albumartist,
             const std::string& album,
             const std::string& year,
             const std::string& track,
             const std::string& genre,
             const std::string& comment,
             int trackLength) override;
  int Encode(int numBytesRead, const uint8_t* stream) override;
  bool Finish() override;

private:
  static FLAC__StreamEncoderWriteStatus write_callback_flac(const FLAC__StreamEncoder *encoder,
                                                            const FLAC__byte buffer[],
                                                            size_t bytes,
                                                            unsigned samples,
                                                            unsigned current_frame,
                                                            void *client_data);
  static FLAC__StreamEncoderSeekStatus seek_callback_flac(const FLAC__StreamEncoder *encoder,
                                                          FLAC__uint64 absolute_byte_offset,
                                                          void *client_data);

  static FLAC__StreamEncoderTellStatus tell_callback_flac(const FLAC__StreamEncoder *encoder,
                                                          FLAC__uint64 *absolute_byte_offset,
                                                          void *client_data);

  int64_t               m_tellPos; ///< position for tell() callback
  FLAC__StreamEncoder*  m_encoder;
  FLAC__StreamMetadata* m_metadata[2];
  FLAC__int32           m_samplesBuf[SAMPLES_BUF_SIZE];
};

CEncoderFlac::CEncoderFlac(KODI_HANDLE instance)
  : CInstanceAudioEncoder(instance),
    m_tellPos(0)
{
  m_metadata[0] = nullptr;
  m_metadata[1] = nullptr;

  m_encoder = FLAC__stream_encoder_new();
  if (m_encoder == nullptr)
    kodi::Log(ADDON_LOG_ERROR, "Failed to construct flac stream encoder");
}

CEncoderFlac::~CEncoderFlac()
{
  // free the metadata
  if (m_metadata[0])
    FLAC__metadata_object_delete(m_metadata[0]);
  if (m_metadata[1])
    FLAC__metadata_object_delete(m_metadata[1]);

  // free the encoder
  if (m_encoder)
    FLAC__stream_encoder_delete(m_encoder);
}

bool CEncoderFlac::Start(int inChannels, int inRate, int inBits,
                         const std::string& title, const std::string& artist,
                         const std::string& albumartist, const std::string& album,
                         const std::string& year, const std::string& track, const std::string& genre,
                         const std::string& comment, int trackLength)
{
  if (!m_encoder)
    return false;

  // we accept only 2 / 44100 / 16 atm
  if (inChannels != 2 || inRate != 44100 || inBits != 16)
  {
    kodi::Log(ADDON_LOG_ERROR, "Invalid input format to encode");
    return false;
  }

  FLAC__bool ok = 1;

  ok &= FLAC__stream_encoder_set_verify(m_encoder, true);
  ok &= FLAC__stream_encoder_set_channels(m_encoder, inChannels);
  ok &= FLAC__stream_encoder_set_bits_per_sample(m_encoder, inBits);
  ok &= FLAC__stream_encoder_set_sample_rate(m_encoder, inRate);
  ok &= FLAC__stream_encoder_set_total_samples_estimate(m_encoder, trackLength / 4);
  ok &= FLAC__stream_encoder_set_compression_level(m_encoder, kodi::GetSettingInt("level"));

  // now add some metadata
  FLAC__StreamMetadata_VorbisComment_Entry entry;
  if (ok)
  {
    if (
      (m_metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == nullptr ||
      (m_metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == nullptr ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", artist.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", album.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUMARTIST", albumartist.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TITLE", title.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "GENRE", genre.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "TRACKNUMBER", track.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "DATE", year.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false) ||
      !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "COMMENT", comment.c_str()) ||
      !FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0], entry, false)
      )
    {
      ok = false;
    }
    else
    {
      m_metadata[1]->length = 4096;
      ok = FLAC__stream_encoder_set_metadata(m_encoder, m_metadata, 2);
    }
  }

  // initialize encoder in stream mode
  if (ok)
  {
    FLAC__StreamEncoderInitStatus init_status;
    init_status = FLAC__stream_encoder_init_stream(m_encoder, write_callback_flac, seek_callback_flac, tell_callback_flac, nullptr, this);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
      ok = false;
    }
  }

  if (!ok)
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to create flac stream encoder");
    return false;
  }

  return true;
}

int CEncoderFlac::Encode(int numBytesRead, const uint8_t* stream)
{
  if (!m_encoder)
    return 0;

  int nLeftSamples = numBytesRead / 2; // each sample takes 2 bytes (16 bits per sample)
  while (nLeftSamples > 0)
  {
    int nSamples = nLeftSamples > SAMPLES_BUF_SIZE ? SAMPLES_BUF_SIZE : nLeftSamples;

    // convert the packed little-endian 16-bit PCM samples into an interleaved FLAC__int32 buffer for libFLAC
    for (int i = 0; i < nSamples; i++)
    { // inefficient but simple and works on big- or little-endian machines.
      m_samplesBuf[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)stream[2*i+1] << 8) | (FLAC__int16)stream[2*i]);
    }

    // feed samples to encoder
    if (!FLAC__stream_encoder_process_interleaved(m_encoder, m_samplesBuf, nSamples / 2))
    {
      return 0;
    }

    nLeftSamples -= nSamples;
    stream += nSamples * 2; // skip processed samples
  }
  return numBytesRead; // consumed everything
}

bool CEncoderFlac::Finish()
{
  if (!m_encoder)
    return false;

  FLAC__stream_encoder_finish(m_encoder);
  return true;
}

FLAC__StreamEncoderWriteStatus CEncoderFlac::write_callback_flac(const FLAC__StreamEncoder *encoder,
                                                                 const FLAC__byte buffer[],
                                                                 size_t bytes,
                                                                 unsigned samples,
                                                                 unsigned current_frame,
                                                                 void *client_data)
{
  CEncoderFlac *context = static_cast<CEncoderFlac*>(client_data);
  if (context)
  {
    if (context->Write(static_cast<const uint8_t*>(buffer), bytes) == bytes)
    {
      context->m_tellPos += bytes;
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }
  }
  return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

FLAC__StreamEncoderSeekStatus CEncoderFlac::seek_callback_flac(const FLAC__StreamEncoder *encoder,
                                                               FLAC__uint64 absolute_byte_offset,
                                                               void *client_data)
{
  CEncoderFlac *context = static_cast<CEncoderFlac*>(client_data);
  if (context)
  {
    if (context->Seek(static_cast<int64_t>(absolute_byte_offset), 0) == absolute_byte_offset)
    {
      context->m_tellPos = absolute_byte_offset;
      return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }
  }
  return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
}

FLAC__StreamEncoderTellStatus CEncoderFlac::tell_callback_flac(const FLAC__StreamEncoder *encoder,
                                                               FLAC__uint64 *absolute_byte_offset,
                                                               void *client_data)
{
  // libFLAC will cope without a real tell callback
  CEncoderFlac *context = static_cast<CEncoderFlac*>(client_data);
  if (context)
  {
    *absolute_byte_offset = context->m_tellPos;
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
  }
  return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
}

//------------------------------------------------------------------------------

class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override;
};

ADDON_STATUS CMyAddon::CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
{
  addonInstance = new CEncoderFlac(instance);
  return ADDON_STATUS_OK;
}

ADDONCREATOR(CMyAddon);
