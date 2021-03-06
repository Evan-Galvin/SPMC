/*
 *      Copyright (C) 2011-2014 Team XBMC
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

#include <stdlib.h>

#include "system.h"
#include <EGL/egl.h>
#include "EGLNativeTypeRKAndroid.h"
#include "utils/log.h"
#include "guilib/gui3d.h"
#include "android/activity/XBMCApp.h"
#include "android/jni/Build.h"
#include "utils/StringUtils.h"
#include "utils/SysfsUtils.h"
#include "utils/RKutils.h"

bool CEGLNativeTypeRKAndroid::CheckCompatibility()
{
  if (StringUtils::StartsWithNoCase(CJNIBuild::HARDWARE, "rk3"))  // Rockchip
  {
    if (SysfsUtils::HasRW("/sys/class/display/display0.HDMI/mode"))
      return true;
    else
      CLog::Log(LOGERROR, "RKEGL: no rw on /sys/class/display/display0.HDMI/mode");
  }
  return false;
}

bool CEGLNativeTypeRKAndroid::GetNativeResolution(RESOLUTION_INFO *res) const
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string mode;
  RESOLUTION_INFO hdmi_res;
  if (SysfsUtils::GetString("/sys/class/display/display0.HDMI/mode", mode) == 0 && rk_mode_to_resolution(mode.c_str(), &hdmi_res))
  {
    m_curHdmiResolution = mode;
    *res = hdmi_res;
    res->iWidth = m_fb_res.iWidth;
    res->iHeight = m_fb_res.iHeight;
    res->iSubtitles = (int)(0.965 * res->iHeight);
  }
  else
    *res = m_fb_res;

  return true;
}

bool CEGLNativeTypeRKAndroid::SetNativeResolution(const RESOLUTION_INFO &res)
{
  switch((int)(res.fRefreshRate*10))
  {
    default:
    case 600:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          return SetDisplayResolution("1280x720p-60");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            return SetDisplayResolution("1920x1080i-60");
          else
            return SetDisplayResolution("1920x1080p-60");
          break;
      }
      break;
    case 500:
      switch(res.iScreenWidth)
      {
        default:
        case 1280:
          return SetDisplayResolution("1280x720p-50");
          break;
        case 1920:
          if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
            return SetDisplayResolution("1920x1080i-50");
          else
            return SetDisplayResolution("1920x1080p-50");
          break;
      }
      break;
    case 300:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k30hz");
          break;
        default:
          return SetDisplayResolution("1920x1080p-30");
          break;
      }
      break;
    case 250:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k25hz");
          break;
        default:
          return SetDisplayResolution("1920x1080p-25");
          break;
      }
      break;
    case 240:
      switch(res.iScreenWidth)
      {
        case 3840:
          return SetDisplayResolution("4k2k24hz");
          break;
        case 4096:
          return SetDisplayResolution("4k2ksmpte");
          break;
        default:
          return SetDisplayResolution("1920x1080p-24");
          break;
      }
      break;
  }

  return false;
}

bool CEGLNativeTypeRKAndroid::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  CEGLNativeTypeAndroid::GetNativeResolution(&m_fb_res);

  std::string valstr;
  if (SysfsUtils::GetString("/sys/class/display/display0.HDMI/modes", valstr) < 0)
    return false;
  std::vector<std::string> probe_str = StringUtils::Split(valstr, "\n");

  resolutions.clear();
  RESOLUTION_INFO res;
  for (size_t i = 0; i < probe_str.size(); i++)
  {
    if(rk_mode_to_resolution(probe_str[i].c_str(), &res))
    {
      res.iWidth = m_fb_res.iWidth;
      res.iHeight = m_fb_res.iHeight;
      res.iSubtitles    = (int)(0.965 * res.iHeight);
      resolutions.push_back(res);
    }
  }
  return resolutions.size() > 0;

}

bool CEGLNativeTypeRKAndroid::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return GetNativeResolution(res);
}

bool CEGLNativeTypeRKAndroid::SetDisplayResolution(const char *resolution)
{
  if (m_curHdmiResolution == resolution)
    return true;

  // switch display resolution
  std::string out = resolution;
  out += '\n';
  if (SysfsUtils::SetString("/sys/class/display/display0.HDMI/mode", out.c_str()) < 0)
    return false;

  m_curHdmiResolution = resolution;

  return true;
}

