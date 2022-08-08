/**
 ********************************************************************
 * @file    test_camera_manager.c
 * @brief
 *
 * @copyright (c) 2021 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <widget_interaction_test/test_widget_interaction.h>
#include <utils/util_misc.h>
#include "test_camera_manager.h"
#include "dji_camera_manager.h"
#include "dji_platform.h"
#include "dji_logger.h"
#include "dji_liveview.h"
/* Private constants ---------------------------------------------------------*/
#define TEST_CAMERA_MANAGER_MEDIA_FILE_NAME_MAX_SIZE             256
#define TEST_CAMERA_MANAGER_MEDIA_DOWNLOAD_FILE_NUM              5

/* Private types -------------------------------------------------------------*/
typedef struct {
    E_DjiCameraType cameraType;
    char *cameraTypeStr;
} T_DjiTestCameraTypeStr;

/* Private values -------------------------------------------------------------*/
static const T_DjiTestCameraTypeStr s_cameraTypeStrList[] = {
    {DJI_CAMERA_TYPE_UNKNOWN, "Unknown"},
    {DJI_CAMERA_TYPE_Z30,     "Zenmuse Z30"},
    {DJI_CAMERA_TYPE_XT2,     "Zenmuse XT2"},
    {DJI_CAMERA_TYPE_PSDK,    "Zenmuse Payload"},
    {DJI_CAMERA_TYPE_XTS,     "Zenmuse XTS"},
    {DJI_CAMERA_TYPE_H20,     "Zenmuse H20"},
    {DJI_CAMERA_TYPE_H20T,    "Zenmuse H20T"},
    {DJI_CAMERA_TYPE_P1,      "Zenmuse P1"},
    {DJI_CAMERA_TYPE_L1,      "Zenmuse L1"},
    {DJI_CAMERA_TYPE_M30,     "Zenmuse M30"},
    {DJI_CAMERA_TYPE_M30T,    "Zenmuse M30T"},
    {DJI_CAMERA_TYPE_H20N,    "Zenmuse H20N"},
};

static FILE *s_downloadMediaFile = NULL;
static T_DjiCameraManagerFileList s_meidaFileList;
static uint32_t downloadStartMs = 0;
static uint32_t downloadEndMs = 0;
static char downloadFileName[TEST_CAMERA_MANAGER_MEDIA_FILE_NAME_MAX_SIZE] = {0};

/* Private functions declaration ---------------------------------------------*/
static uint8_t DjiTest_CameraManagerGetCameraTypeIndex(E_DjiCameraType cameraType);
static T_DjiReturnCode DjiTest_CameraManagerMediaDownloadAndDeleteMediaFile(E_DjiMountPosition position);
static T_DjiReturnCode DjiTest_CameraManagerDownloadFileDataCallback(T_DjiDownloadFilePacketInfo packetInfo,
                                                                     const uint8_t *data, uint16_t len);

/* Exported functions definition ---------------------------------------------*/
/*! @brief Sample to set EV for camera, using async api
 *
 *  @note In this interface, ev will be got then be set.
 *  In order to use this function, the camera exposure mode should be
 *  set to be DJI_CAMERA_MANAGER_EXPOSURE_MODE_PROGRAM_AUTO,
 *  DJI_CAMERA_MANAGER_EXPOSURE_MODE_SHUTTER_PRIORITY or
 *  DJI_CAMERA_MANAGER_EXPOSURE_MODE_APERTURE_PRIORITY first
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param dataTarget the target exposure mode
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetEV(E_DjiMountPosition position,
                                           E_DjiCameraManagerExposureCompensation exposureCompensation)
{
    T_DjiReturnCode returnCode;
    E_DjiCameraManagerExposureCompensation exposureCompensationTemp;

    returnCode = DjiCameraManager_GetExposureCompensation(position, &exposureCompensationTemp);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d exposure compensation failed, error code: 0x%08X.",
                       position, returnCode);
    }

    if (exposureCompensationTemp == exposureCompensation) {
        USER_LOG_INFO("The mount position %d camera's exposure compensation is already what you expected.",
                      position);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiCameraManager_SetExposureCompensation(position, exposureCompensation);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's exposure compensation %d failed,"
                       "error code: 0x%08X.", position, exposureCompensation, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set exposure mode for camera, using async api
 *
 *  @note In this interface, exposure will be got then be set.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param dataTarget the target exposure mode
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetExposureMode(E_DjiMountPosition position,
                                                     E_DjiCameraManagerExposureMode exposureMode)
{
    T_DjiReturnCode returnCode;
    E_DjiCameraManagerExposureMode exposureModeTemp;

    returnCode = DjiCameraManager_GetExposureMode(position, &exposureModeTemp);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d exposure mode failed, error code: 0x%08X",
                       position, returnCode);
        return returnCode;
    }

    if (exposureModeTemp == exposureMode) {
        USER_LOG_INFO("The mounted position %d camera's exposure mode is already what you expected.",
                      position);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiCameraManager_SetExposureMode(position, exposureMode);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's exposure mode %d failed, current exposure is %d,"
                       " error code: 0x%08X", position, exposureMode, exposureModeTemp, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set ISO value for camera, using async api
 *
 *  @note In this interface, ISO will be got then be set.
 *  For the X5, X5R, X4S and X5S, the ISO value can be set for all
 *  modes. For the other cameras, the ISO value can only be set when
 *  the camera exposure mode is in Manual mode
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param dataTarget the target ISO value
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetISO(E_DjiMountPosition position,
                                            E_DjiCameraManagerISO isoData)
{
    T_DjiReturnCode returnCode;
    E_DjiCameraManagerISO isoDataTemp;

    returnCode = DjiCameraManager_GetISO(position, &isoDataTemp);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d camera's iso failed, error code: 0x%08X.",
                       position, returnCode);
        return returnCode;
    }

    if (isoDataTemp == isoData) {
        USER_LOG_INFO("The mounted position %d camera's iso is already what you expected.",
                      position);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiCameraManager_SetISO(position, isoData);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's iso %d failed, "
                       "error code: 0x%08X.", position, isoData, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set shutter speed for camera, using async api
 *
 *  @note In this interface, shutter speed will be got then be set.
 *  The shutter speed can be set only when the camera exposure mode
 *  is Shutter mode or Manual mode. The shutter speed should not be
 *  set slower than the video frame rate when the camera's mode is
 *  RECORD_VIDEO.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param dataTarget the target shutter speed
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetShutterSpeed(E_DjiMountPosition position,
                                                     E_DjiCameraManagerShutterSpeed shutterSpeed)
{
    T_DjiReturnCode returnCode;
    E_DjiCameraManagerShutterSpeed shutterSpeedTemp;

    returnCode = DjiCameraManager_GetShutterSpeed(position, &shutterSpeedTemp);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d camera's shutter speed failed, "
                       "error code: 0x%08X.", position, returnCode);
        return returnCode;
    }

    if (shutterSpeedTemp == shutterSpeed) {
        USER_LOG_INFO("The mounted position %d camera's shutter speed is already what you expected.",
                      position);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiCameraManager_SetShutterSpeed(position, shutterSpeed);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's shutter speed %d failed, "
                       "error code: 0x%08X.", position, shutterSpeed, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set shutter aperture value for camera, using async api
 *
 *  @note In this interface, aperture value will be got then be set.
 *  In order to use this function, the exposure must be in MANUAL or APERTURE_PRIORITY.
*   Supported only by the X5, X5R, X4S, X5S camera.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param dataTarget the target aperture value
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetAperture(E_DjiMountPosition position,
                                                 E_DjiCameraManagerAperture aperture)
{
    T_DjiReturnCode returnCode;
    E_DjiCameraManagerAperture apertureTemp;

    returnCode = DjiCameraManager_GetAperture(position, &apertureTemp);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d camera's aperture failed, "
                       "error code: 0x%08X.", position, returnCode);
        return returnCode;
    }

    if (apertureTemp == aperture) {
        USER_LOG_INFO("The mounted position %d camera's aperture is already what you expected.",
                      position);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    }

    returnCode = DjiCameraManager_SetAperture(position, aperture);

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's aperture %d failed, "
                       "error code: 0x%08X.", position, aperture, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set focus point for camera, using async api
 *
 *  @note In this interface, focus mode will be set to be AUTO. Then the
 * focus point will be set to be (x, y).
 * When the focus mode is auto, the target point is the focal point.
 * When the focus mode is manual, the target point is the zoom out area
 * if the focus assistant is enabled for the manual mode. Supported only
 * by the X5, X5R, Z3 cameras, Mavic Pro camera, Phantom 4 Pro camera,
 * Mavic 2 Pro, Mavic 2 Zoom Camera, Mavic 2 Enterprise Camera, X5S. "
 * It's should be attention that X4S will keep focus point as (0.5,0.5) "
 * all the time, the setting of focus point to X4S will quickly replaced "
 * by (0.5, 0.5).
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param x the x value of target focus point, 0~1
 *  @param y the y value of target focus point, 0~1
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetFocusPoint(E_DjiMountPosition position,
                                                   T_DjiCameraManagerFocusPosData focusPoint)
{
    T_DjiReturnCode returnCode;

    /*!< set camera focus mode to be CameraModule::FocusMode::AUTO */
    USER_LOG_INFO("Set mounted position %d camera's focus mode to auto mode.",
                  position);
    returnCode = DjiCameraManager_SetFocusMode(position, DJI_CAMERA_MANAGER_FOCUS_MODE_AUTO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's focus mode(%d) failed,"
                       " error code :0x%08X.", position, DJI_CAMERA_MANAGER_FOCUS_MODE_AUTO,
                       returnCode);
        return returnCode;
    }

    USER_LOG_INFO("Set mounted position %d camera's focus point to (%0.1f, %0.1f).",
                  position, focusPoint.focusX, focusPoint.focusY);
    returnCode = DjiCameraManager_SetFocusTarget(position, focusPoint);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's focus point(%0.1f, %0.1f) failed,"
                       " error code :0x%08X.", position, focusPoint.focusX, focusPoint.focusY,
                       returnCode);
    }

    return returnCode;
}

/*! @brief Sample to set tap-zoom point for camera, using async api
 *
 *  @note In this interface, tap-zoom function will be enable and the
 * multiplier will be set. Then the tap-zoom function will start with the
 * target tap-zoom point (x, y)
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param multiplier the zoom multiplier of each tap zoom
 *  @param x the x value of target tap-zoom point, 0~1
 *  @param y the y value of target tap-zoom point, 0~1
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerSetTapZoomPoint(E_DjiMountPosition position, uint8_t multiplier,
                                                     T_DjiCameraManagerTapZoomPosData tapZoomPosData)
{
    T_DjiReturnCode returnCode;

    /*!< set camera tap zoom enable parameter to be enable */
    USER_LOG_INFO("Enable mounted position %d camera's tap zoom status.",
                  position);
    returnCode = DjiCameraManager_SetTapZoomEnabled(position, true);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Enable mounted position %d camera's tap zoom function failed,"
                       " error code :0x%08X.", position, returnCode);
        return returnCode;
    }

    /*!< set camera tap zoom multiplier parameter */
    USER_LOG_INFO("Set mounted position %d camera's tap zoom multiplier to %d x.",
                  position, multiplier);
    returnCode = DjiCameraManager_SetTapZoomMultiplier(position, multiplier);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's tap zoom multiplier(%d) failed,"
                       " error code :0x%08X.", position, multiplier, returnCode);
        return returnCode;
    }

    USER_LOG_INFO("Set mounted position %d camera's tap zoom point to (%f, %f).",
                  position, tapZoomPosData.focusX, tapZoomPosData.focusY);
    returnCode = DjiCameraManager_TapZoomAtTarget(position, tapZoomPosData);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Set mounted position %d camera's tap zoom target point(%f ,%f) failed,"
                       " error code :0x%08X.", position, tapZoomPosData.focusX, tapZoomPosData.focusY,
                       returnCode);
    }

    return returnCode;
}

/*! @brief Sample to execute position zoom on camera, using sync api
 *
 *  @note It is only supported by X5, X5R and X5S camera on Osmo with lens
 * Olympus M.Zuiko ED 14-42mm f/3.5-5.6 EZ, Z3 camera, Z30 camera.
 *  @note In this interface, the zoom will set the zoom factor as the your
 * target value.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param factor target zoom factor
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerOpticalZoom(E_DjiMountPosition position,
                                                 E_DjiCameraZoomDirection zoomDirection,
                                                 dji_f32_t factor)
{
    T_DjiReturnCode returnCode;
    T_DjiCameraManagerOpticalZoomParam opticalZoomParam;

    returnCode = DjiCameraManager_GetOpticalZoomParam(position, &opticalZoomParam);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Get mounted position %d camera's zoom param failed, error code :0x%08X",
                       position, returnCode);
        return returnCode;
    }

    USER_LOG_INFO("The mounted position %d camera's current optical zoom factor is:%0.1f x, "
                  "max optical zoom factor is :%0.1f x", position, opticalZoomParam.currentOpticalZoomFactor,
                  opticalZoomParam.maxOpticalZoomFactor);

    USER_LOG_INFO("Set mounted position %d camera's zoom factor: %0.1f x.", position, factor);
    returnCode = DjiCameraManager_SetOpticalZoomParam(position, zoomDirection, factor);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_INFO("Set mounted position %d camera's zoom factor(%0.1f) failed, error code :0x%08X",
                      position, factor, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to execute continuous zoom on camera, using sync api
 *
 *  @note It is only supported by X5, X5R and X5S camera on Osmo with lens
 * Olympus M.Zuiko ED 14-42mm f/3.5-5.6 EZ, Z3 camera, Z30 camera.
 *  @note In this interface, the zoom will start with the designated direction
 * and speed, and will stop after zoomTimeInSecond second(s).
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param direction the choice of zoom out or zoom in
 *  @param speed zooming speed
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartContinuousZoom(E_DjiMountPosition position,
                                                         E_DjiCameraZoomDirection zoomDirection,
                                                         E_DjiCameraZoomSpeed zoomSpeed)
{
    T_DjiReturnCode returnCode;

    //    USER_LOG_INFO("Mounted position %d camera start continuous optical zoom.\r\n", position);
    returnCode = DjiCameraManager_StartContinuousOpticalZoom(position, zoomDirection, zoomSpeed);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera start continuous zoom  failed,"
                       " error code :0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to stop continuous zoom on camera, using async api
 *
 *  @note It is only supported by X5, X5R and X5S camera on Osmo with lens
 * Olympus M.Zuiko ED 14-42mm f/3.5-5.6 EZ, Z3 camera, Z30 camera.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStopContinuousZoom(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;

//    USER_LOG_INFO("Mounted position %d camera stop continuous optical zoom.\r\n", position);
    returnCode = DjiCameraManager_StopContinuousOpticalZoom(position);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera stop continuous zoom failed,"
                       " error code :0x%08X", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to shoot single photo, using async api
 *
 *  @note In this interface, camera will be set to be the SHOOT_PHOTO mode
 * then start to shoot a single photo.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartShootSinglePhoto(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    /*!< set camera work mode as shoot photo */
    USER_LOG_INFO("Set mounted position %d camera's work mode as shoot-photo mode", position);
    returnCode = DjiCameraManager_SetMode(position, DJI_CAMERA_MANAGER_WORK_MODE_SHOOT_PHOTO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's work mode as shoot-photo mode failed,"
                       " error code :0x%08X", position, returnCode);
        return returnCode;
    }

    /*!< set shoot-photo mode */
    USER_LOG_INFO("Set mounted position %d camera's shoot photo mode as single-photo mode", position);
    returnCode = DjiCameraManager_SetShootPhotoMode(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_SINGLE);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's shoot photo mode as single-photo mode failed,"
                       " error code :0x%08X", position, returnCode);
        return returnCode;
    }

    /*! wait the APP change the shoot-photo mode display */
    USER_LOG_INFO("Sleep 0.5s...");
    osalHandler->TaskSleepMs(500);

    /*!< start to shoot single photo */
    USER_LOG_INFO("Mounted position %d camera start to shoot photo", position);
    returnCode = DjiCameraManager_StartShootPhoto(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_SINGLE);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Mounted position %d camera shoot photo failed, "
                       "error code :0x%08X", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to shoot burst photo, using async api
 *
 *  @note In this interface, camera will be set to be the SHOOT_PHOTO mode
 * then start to shoot a burst photo.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param count The number of pictures in each burst shooting
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartShootBurstPhoto(E_DjiMountPosition position,
                                                          E_DjiCameraBurstCount burstCount)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    /*!< set camera work mode as shoot photo */
    USER_LOG_INFO("set mounted position %d camera's work mode as shoot photo mode.", position);
    returnCode = DjiCameraManager_SetMode(position, DJI_CAMERA_MANAGER_WORK_MODE_SHOOT_PHOTO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set mounted position %d camera's work mode as shoot photo mode failed,"
                       " error code :0x%08X.", position, returnCode);
        return returnCode;
    }

    /*!< set shoot-photo mode */
    USER_LOG_INFO("Set mounted position %d camera's shoot photo mode as burst-photo mode", position);
    returnCode = DjiCameraManager_SetShootPhotoMode(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_BURST);
    if (returnCode == DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        return returnCode;
    }

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set mounted position %d camera's shoot photo mode as burst-photo mode failed,"
                       " error code :0x%08X", position, returnCode);
        return returnCode;
    }

    /*! wait the APP change the shoot-photo mode display */
    USER_LOG_INFO("Sleep 0.5s...");
    osalHandler->TaskSleepMs(500);

    /*!< set shoot-photo mode parameter */
    USER_LOG_INFO("Set mounted position %d camera's burst count to %d", position, burstCount);
    returnCode = DjiCameraManager_SetPhotoBurstCount(position, burstCount);

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's burst count(%d) failed,"
                       " error code :0x%08X.", position, burstCount, returnCode);
        return returnCode;
    }
    /*!< start to shoot single photo */
    USER_LOG_INFO("Mounted position %d camera start to shoot photo.", position);
    returnCode = DjiCameraManager_StartShootPhoto(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_BURST);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Mounted position %d camera shoot photo failed, "
                       "error code :0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to shoot AEB photo, using async api
 *
 *  @note In this interface, camera will be set to be the SHOOT_PHOTO mode
 * then start to shoot a AEB photo.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param photoNum The number of pictures in each AEB shooting
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartShootAEBPhoto(E_DjiMountPosition position,
                                                        E_DjiCameraManagerPhotoAEBCount aebCount)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    /*!< set camera work mode as shoot photo */
    USER_LOG_INFO("set mounted position %d camera's work mode as shoot photo mode.", position);
    returnCode = DjiCameraManager_SetMode(position, DJI_CAMERA_MANAGER_WORK_MODE_SHOOT_PHOTO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's work mode as shoot photo mode failed,"
                       " error code :0x%08X.", position, returnCode);
        return returnCode;
    }

    /*!< set shoot-photo mode */
    USER_LOG_INFO("Set mounted position %d camera's shoot photo mode as AEB-photo mode", position);
    returnCode = DjiCameraManager_SetShootPhotoMode(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_AEB);
    if (returnCode == DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        return returnCode;
    }

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set mounted position %d camera's shoot photo mode as AEB-photo mode failed,"
                       " error code :0x%08X.", position, returnCode);
        return returnCode;
    }

    /*! wait the APP change the shoot-photo mode display */
    USER_LOG_INFO("Sleep 0.5s...");
    osalHandler->TaskSleepMs(500);

    /*!< set shoot-photo mode parameter */
    USER_LOG_INFO("Set mounted position %d camera's AEB count to %d", position, aebCount);
    returnCode = DjiCameraManager_SetPhotoAEBCount(position, aebCount);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's AEB count(%d) failed,"
                       " error code :0x%08X.", position, aebCount, returnCode);
        return returnCode;
    }
    /*!< start to shoot single photo */
    USER_LOG_INFO("Mounted position %d camera start to shoot photo.", position);
    returnCode = DjiCameraManager_StartShootPhoto(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_AEB);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Mounted position %d camera shoot photo failed, "
                       "error code :0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to start shooting interval photo, using async api
 *
 *  @note In this interface, camera will be set to be the SHOOT_PHOTO mode
 * then start to shoot a interval photo.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @param intervalData the parameter of interval shooting
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartShootIntervalPhoto(E_DjiMountPosition position,
                                                             T_DjiCameraPhotoTimeIntervalSettings intervalData)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    /*!< set camera work mode as shoot photo */
    USER_LOG_INFO("set mounted position %d camera's work mode as shoot photo mode.", position);
    returnCode = DjiCameraManager_SetMode(position, DJI_CAMERA_MANAGER_WORK_MODE_SHOOT_PHOTO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set mounted position %d camera's work mode as shoot photo mode failed,"
                       " error code :0x%08X.", position, returnCode);
        return returnCode;
    }

    /*!< set shoot-photo mode */
    USER_LOG_INFO("Set mounted position %d camera's shoot photo mode as interval-photo mode", position);
    returnCode = DjiCameraManager_SetShootPhotoMode(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_INTERVAL);
    if (returnCode == DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        return returnCode;
    }

    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set mounted position %d camera's shoot photo mode as interval-photo mode failed,"
                       " error code :0x%08X", position, returnCode);
        return returnCode;
    }

    /*! wait the APP change the shoot-photo mode display */
    USER_LOG_INFO("Sleep 0.5s...");
    osalHandler->TaskSleepMs(500);

    /*!< set shoot-photo mode parameter */
    USER_LOG_INFO("Set mounted position %d camera's interval time to %d s", position, intervalData.timeIntervalSeconds);
    returnCode = DjiCameraManager_SetPhotoTimeIntervalSettings(position, intervalData);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's time interval parameter"
                       "(photo number:%d, time interval:%d) failed, error code :0x%08X.",
                       position, intervalData.captureCount, intervalData.timeIntervalSeconds, returnCode);
        return returnCode;
    }

    /*! wait the APP change the shoot-photo mode display */
    USER_LOG_INFO("Sleep 0.5s...");
    osalHandler->TaskSleepMs(500);

    /*!< start to shoot single photo */
    USER_LOG_INFO("Mounted position %d camera start to shoot photo.", position);
    returnCode = DjiCameraManager_StartShootPhoto(position, DJI_CAMERA_MANAGER_SHOOT_PHOTO_MODE_INTERVAL);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera shoot photo failed, "
                       "error code :0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to stop shooting, using async api
 *
 *  @note In this interface, camera will stop all the shooting action
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStopShootPhoto(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;

    USER_LOG_INFO("Mounted position %d camera stop to shoot photo.", position);
    returnCode = DjiCameraManager_StopShootPhoto(position);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera stop to shoot photo failed,"
                       " error code:0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to start record video, using async api
 *
 *  @note In this interface, camera will be set to be the RECORD_VIDEO mode
 * then start to record video.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStartRecordVideo(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;
    /*!< set camera work mode as record video */
    USER_LOG_INFO("set mounted position %d camera's work mode as record-video mode", position);
    returnCode = DjiCameraManager_SetMode(position, DJI_CAMERA_MANAGER_WORK_MODE_RECORD_VIDEO);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("set mounted position %d camera's work mode as record-video mode failed,"
                       " error code :0x%08X", position, returnCode);
        return returnCode;
    }

    /*!< start to take video */
    USER_LOG_INFO("Mounted position %d camera start to record video.", position);
    returnCode = DjiCameraManager_StartRecordVideo(position);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera start to record video failed,"
                       " error code:0x%08X.", position, returnCode);
    }

    return returnCode;
}

/*! @brief Sample to stop record video, using async api
 *
 *  @note In this interface, camera will be set to be the RECORD_VIDEO mode
 * then stop recording video.
 *  @param index payload node index, input limit see enum
 * DJI::OSDK::PayloadIndexType
 *  @return T_DjiReturnCode error code
 */
T_DjiReturnCode DjiTest_CameraManagerStopRecordVideo(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;
    USER_LOG_INFO("Mounted position %d camera stop to record video.", position);
    returnCode = DjiCameraManager_StopRecordVideo(position);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS &&
        returnCode != DJI_ERROR_CAMERA_MANAGER_MODULE_CODE_UNSUPPORTED_COMMAND) {
        USER_LOG_ERROR("Mounted position %d camera stop to record video failed,"
                       " error code:0x%08X.", position, returnCode);
    }

    return returnCode;
}

T_DjiReturnCode DjiTest_CameraManagerRunSample(E_DjiMountPosition mountPosition,
                                               E_DjiTestCameraManagerSampleSelect cameraManagerSampleSelect)
{
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    T_DjiReturnCode returnCode;
    E_DjiCameraType cameraType;
    T_DjiCameraManagerFirmwareVersion firmwareVersion;
    T_DjiCameraManagerFocusPosData focusPosData;
    T_DjiCameraManagerTapZoomPosData tapZoomPosData;

    USER_LOG_INFO("Camera manager sample start");
    DjiTest_WidgetLogAppend("Camera manager sample start");

    USER_LOG_INFO("--> Step 1: Init camera manager module");
    DjiTest_WidgetLogAppend("--> Step 1: Init camera manager module");
    returnCode = DjiCameraManager_Init();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Init camera manager failed, error code: 0x%08X\r\n", returnCode);
        goto exitCameraModule;
    }

    USER_LOG_INFO("--> Step 2: Get camera type and version");
    DjiTest_WidgetLogAppend("--> Step 2: Get camera type and version");
    returnCode = DjiCameraManager_GetCameraType(mountPosition, &cameraType);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get mounted position %d camera's type failed, error code: 0x%08X\r\n",
                       mountPosition, returnCode);
        goto exitCameraModule;
    }
    USER_LOG_INFO("Mounted position %d camera's type is %s",
                  mountPosition,
                  s_cameraTypeStrList[DjiTest_CameraManagerGetCameraTypeIndex(cameraType)].cameraTypeStr);

    returnCode = DjiCameraManager_GetFirmwareVersion(mountPosition, &firmwareVersion);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get mounted position %d camera's firmware version failed, error code: 0x%08X\r\n",
                       mountPosition, returnCode);
        goto exitCameraModule;
    }
    USER_LOG_INFO("Mounted position %d camera's firmware is V%02d.%02d.%02d.%02d\r\n", mountPosition,
                  firmwareVersion.firmware_version[0], firmwareVersion.firmware_version[1],
                  firmwareVersion.firmware_version[2], firmwareVersion.firmware_version[3]);

    E_DjiLiveViewCameraSource liveViewCameraSource;

    if (cameraType == DJI_CAMERA_TYPE_H20 || cameraType == DJI_CAMERA_TYPE_H20T) {
        USER_LOG_INFO("--> Step 3: Change camera's live view source");
        DjiTest_WidgetLogAppend("--> Step 3: Change camera's live view source");

        USER_LOG_INFO("Init live view.");
        returnCode = DjiLiveview_Init();
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Init live view failed, error code: 0x%08X\r\n", returnCode);
            goto exitLiveViewModule;
        }

        USER_LOG_INFO("Set mounted position %d camera's live view source to zoom.\r\n",
                      mountPosition);
        liveViewCameraSource = (cameraType == DJI_CAMERA_TYPE_H20) ? DJI_LIVEVIEW_CAMERA_SOURCE_H20_ZOOM :
                               DJI_LIVEVIEW_CAMERA_SOURCE_H20T_ZOOM;
        returnCode = DjiLiveview_StartH264Stream((uint8_t) mountPosition, liveViewCameraSource, NULL);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Set mounted position %d camera's live view source failed,"
                           "error code: 0x%08X\r\n", mountPosition, returnCode);
            goto exitLiveViewSource;
        }
    }

    switch (cameraManagerSampleSelect) {
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_SHUTTER_SPEED: {
            USER_LOG_INFO("--> Function a: Set camera shutter speed to 1/100 s");
            DjiTest_WidgetLogAppend("--> Function a: Set camera shutter speed to 1/100 s");
            if (cameraType == DJI_CAMERA_TYPE_H20 || cameraType == DJI_CAMERA_TYPE_H20T ||
                cameraType == DJI_CAMERA_TYPE_M30 || cameraType == DJI_CAMERA_TYPE_M30T) {
                USER_LOG_INFO("Set mounted position %d camera's exposure mode to manual mode.",
                              mountPosition);
                returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                                  DJI_CAMERA_MANAGER_EXPOSURE_MODE_EXPOSURE_MANUAL);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                                   "error code: 0x%08X\r\n", mountPosition, returnCode);
                    goto exitLiveViewSource;
                }
            } else {
                USER_LOG_INFO("Set mounted position %d camera's exposure mode to shutter priority mode.",
                              mountPosition);
                returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                                  DJI_CAMERA_MANAGER_EXPOSURE_MODE_SHUTTER_PRIORITY);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                                   "error code: 0x%08X\r\n", mountPosition, returnCode);
                    goto exitLiveViewSource;
                }
            }

            USER_LOG_INFO("Set mounted position %d camera's shutter speed to 1/100 s.",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetShutterSpeed(mountPosition,
                                                              DJI_CAMERA_MANAGER_SHUTTER_SPEED_1_100);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's shutter speed failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_APERTURE: {
            USER_LOG_INFO("--> Function b: Set camera aperture to 400(F/4)");
            DjiTest_WidgetLogAppend("--> Function b: Set camera aperture to 400(F/4)");
            if (cameraType == DJI_CAMERA_TYPE_H20 || cameraType == DJI_CAMERA_TYPE_H20T
                || cameraType == DJI_CAMERA_TYPE_M30 || cameraType == DJI_CAMERA_TYPE_M30T) {
                USER_LOG_INFO("Set mounted position %d camera's exposure mode to manual mode.",
                              mountPosition);
                returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                                  DJI_CAMERA_MANAGER_EXPOSURE_MODE_EXPOSURE_MANUAL);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                                   "error code: 0x%08X\r\n", mountPosition, returnCode);
                    goto exitLiveViewSource;
                }
            } else {
                USER_LOG_INFO("Set mounted position %d camera's exposure mode to aperture priority mode.",
                              mountPosition);
                returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                                  DJI_CAMERA_MANAGER_EXPOSURE_MODE_APERTURE_PRIORITY);
                if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                    USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                                   "error code: 0x%08X\r\n", mountPosition, returnCode);
                    goto exitLiveViewSource;
                }
            }

            USER_LOG_INFO("Set mounted position %d camera's aperture to 400(F/4).",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetAperture(mountPosition, DJI_CAMERA_MANAGER_APERTURE_F_4);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's aperture failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_EV: {
            USER_LOG_INFO("--> Function c: Set camera ev value to +0.3ev");
            DjiTest_WidgetLogAppend("--> Function c: Set camera ev value to +0.3ev");
            USER_LOG_INFO("Set mounted position %d camera's exposure mode to auto mode.",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                              DJI_CAMERA_MANAGER_EXPOSURE_MODE_PROGRAM_AUTO);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Set mounted position %d camera's ev value to +0.3ev.",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetEV(mountPosition, DJI_CAMERA_MANAGER_EXPOSURE_COMPENSATION_P_0_3);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's EV failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_ISO: {
            USER_LOG_INFO("--> Function d: Set camera ISO value to 1600");
            DjiTest_WidgetLogAppend("--> Function d: Set camera ISO value to 1600");
            USER_LOG_INFO("Set mounted position %d camera's exposure mode to manual mode.",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetExposureMode(mountPosition,
                                                              DJI_CAMERA_MANAGER_EXPOSURE_MODE_EXPOSURE_MANUAL);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's exposure mode failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Set mounted position %d camera's ISO value to 1600.",
                          mountPosition);
            returnCode = DjiTest_CameraManagerSetISO(mountPosition, DJI_CAMERA_MANAGER_ISO_1600);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's iso failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_FOCUS_POINT: {
            USER_LOG_INFO("--> Function e: Set camera focus point to (0.3,0.4)");
            DjiTest_WidgetLogAppend("--> Function e: Set camera focus point to (0.3,0.4)");
            focusPosData.focusX = 0.3f;
            focusPosData.focusY = 0.4f;
            returnCode = DjiTest_CameraManagerSetFocusPoint(mountPosition, focusPosData);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's focus point(0.8,0.8) failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_TAP_ZOOM_POINT: {
            USER_LOG_INFO("--> Function f: Set camera tap zoom point from (5x, 0.3m, 0.3m) to (4x, 0.8m, 0.7m)");
            DjiTest_WidgetLogAppend(
                "--> Function f: Set camera tap zoom point from (5x, 0.3m, 0.3m) to (4x, 0.8m, 0.7m)");
            tapZoomPosData.focusX = 0.3f;
            tapZoomPosData.focusY = 0.3f;
            returnCode = DjiTest_CameraManagerSetTapZoomPoint(mountPosition, 5, tapZoomPosData);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's tap zoom point(5, 0.3m,0.3m) failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Sleep 5s...");
            osalHandler->TaskSleepMs(5000);

            tapZoomPosData.focusX = 0.8f;
            tapZoomPosData.focusY = 0.7f;
            returnCode = DjiTest_CameraManagerSetTapZoomPoint(mountPosition, 4, tapZoomPosData);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's tap zoom point(4, 0.8m,0.7m) failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SET_CAMERA_ZOOM_PARAM: {
            USER_LOG_INFO("--> Function g: Camera zoom from 10x to -5x");
            DjiTest_WidgetLogAppend("--> Function g: Camera zoom from 10x to -5x");
            returnCode = DjiTest_CameraManagerOpticalZoom(mountPosition, DJI_CAMERA_ZOOM_DIRECTION_IN,
                                                          10);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's optical zoom factor 10x failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            USER_LOG_INFO("Sleep 4s...");
            osalHandler->TaskSleepMs(4000);

            returnCode = DjiTest_CameraManagerOpticalZoom(mountPosition, DJI_CAMERA_ZOOM_DIRECTION_OUT, 5);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Set mounted position %d camera's optical zoom factor -5x failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            USER_LOG_INFO("Sleep 4s...");
            osalHandler->TaskSleepMs(4000);

            USER_LOG_INFO(
                "Mounted position %d camera start continuous zoom with zoom-out direction and normal zoom speed.",
                mountPosition);
            returnCode = DjiTest_CameraManagerStartContinuousZoom(mountPosition,
                                                                  DJI_CAMERA_ZOOM_DIRECTION_OUT,
                                                                  DJI_CAMERA_ZOOM_SPEED_NORMAL);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera start continuous zoom failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Sleep 8s...");
            osalHandler->TaskSleepMs(8000);

            USER_LOG_INFO("Mounted position %d camera stop continuous zoom.", mountPosition);
            returnCode = DjiTest_CameraManagerStopContinuousZoom(mountPosition);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera stop continuous zoom failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SHOOT_SINGLE_PHOTO: {
            USER_LOG_INFO("--> Function h: Shoot Single photo");
            DjiTest_WidgetLogAppend("--> Function h: Shoot Single photo");
            returnCode = DjiTest_CameraManagerStartShootSinglePhoto(mountPosition);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera shoot single photo failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SHOOT_AEB_PHOTO: {
            USER_LOG_INFO("--> Function i: Shoot AEB photo with 3 pictures in one time");
            DjiTest_WidgetLogAppend("--> Function i: Shoot AEB photo with 3 pictures in one time");
            returnCode = DjiTest_CameraManagerStartShootAEBPhoto(mountPosition, DJI_CAMERA_MANAGER_PHOTO_AEB_COUNT_3);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera shoot AEB photo failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SHOOT_BURST_PHOTO: {
            USER_LOG_INFO("--> Function j: Shoot Burst photo with 5 pictures in one time");
            DjiTest_WidgetLogAppend("--> Function j: Shoot Burst photo with 5pictures in one time");
            returnCode = DjiTest_CameraManagerStartShootBurstPhoto(mountPosition,
                                                                   DJI_CAMERA_BURST_COUNT_5);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera shoot burst photo failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_SHOOT_INTERVAL_PHOTO: {
            USER_LOG_INFO("--> Function k: Shoot Interval photo with 3s intervals in 15s");
            DjiTest_WidgetLogAppend("--> Function k: Shoot Interval photo with 3s intervals in 15s");
            T_DjiCameraPhotoTimeIntervalSettings intervalData;
            intervalData.captureCount = 255;
            intervalData.timeIntervalSeconds = 3;

            returnCode = DjiTest_CameraManagerStartShootIntervalPhoto(mountPosition, intervalData);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera shoot internal photo failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Sleep 15s...");
            osalHandler->TaskSleepMs(15000);

            returnCode = DjiTest_CameraManagerStopShootPhoto(mountPosition);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera stop shoot photo failed,"
                               "error code: 0x%08X\r\n", mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_RECORD_VIDEO: {
            USER_LOG_INFO("--> Function l: Record video in next 10s");
            DjiTest_WidgetLogAppend("--> Function l: Record video in next 10s");
            returnCode = DjiTest_CameraManagerStartRecordVideo(mountPosition);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera start record video failed, error code: 0x%08X\r\n",
                               mountPosition, returnCode);
                goto exitLiveViewSource;
            }

            USER_LOG_INFO("Sleep 10s...");
            osalHandler->TaskSleepMs(10000);

            returnCode = DjiTest_CameraManagerStopRecordVideo(mountPosition);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Mounted position %d camera stop record video failed, error code: 0x%08X\r\n",
                               mountPosition, returnCode);
                goto exitLiveViewSource;
            }
            break;
        }
        case E_DJI_TEST_CAMERA_MANAGER_SAMPLE_SELECT_DOWNLOAD_AND_DELETE_MEDIA_FILE:
#ifdef SYSTEM_ARCH_LINUX
            DjiTest_CameraManagerMediaDownloadAndDeleteMediaFile(mountPosition);
#else
            USER_LOG_WARN("This feature does not support RTOS platform.");
#endif
            break;
        default: {
            USER_LOG_ERROR("There is no valid command input!");
            break;
        }
    }

exitLiveViewSource:
    if (cameraType == DJI_CAMERA_TYPE_H20 || cameraType == DJI_CAMERA_TYPE_H20T) {
        returnCode = DjiLiveview_StopH264Stream((uint8_t) mountPosition);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Stop mounted position %d camera's live view failed,"
                           "error code: 0x%08X\r\n", mountPosition, returnCode);
        }
    }

exitLiveViewModule:
    if (cameraType == DJI_CAMERA_TYPE_H20 || cameraType == DJI_CAMERA_TYPE_H20T) {
        returnCode = DjiLiveview_Deinit();
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Deinit live view failed, error code: 0x%08X\r\n", returnCode);
        }
    }

exitCameraModule:
    returnCode = DjiCameraManager_DeInit();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Camera manager deinit failed ,error code :0x%08X", returnCode);
    }

    USER_LOG_INFO("Camera manager sample end");
    DjiTest_WidgetLogAppend("Camera manager sample end");
    return returnCode;
}

/* Private functions definition-----------------------------------------------*/
static uint8_t DjiTest_CameraManagerGetCameraTypeIndex(E_DjiCameraType cameraType)
{
    uint8_t i;

    for (i = 0; i < sizeof(s_cameraTypeStrList) / sizeof(s_cameraTypeStrList[0]); i++) {
        if (s_cameraTypeStrList[i].cameraType == cameraType) {
            return i;
        }
    }

    return 0;
}

static T_DjiReturnCode DjiTest_CameraManagerMediaDownloadAndDeleteMediaFile(E_DjiMountPosition position)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();
    uint16_t downloadCount = 0;

    returnCode = DjiCameraManager_RegDownloadFileDataCallback(position, DjiTest_CameraManagerDownloadFileDataCallback);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Register download file data callback failed, error code: 0x%08X.", returnCode);
        return returnCode;
    }

    returnCode = DjiCameraManager_DownloadFileList(position, &s_meidaFileList);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Download file list failed, error code: 0x%08X.", returnCode);
        return returnCode;
    }

    if (s_meidaFileList.totalCount > 0) {
        downloadCount = s_meidaFileList.totalCount;
        printf(
            "\033[1;33;40m -> Download file list finished, total file count is %d, the following %d is list details: \033[0m\r\n",
            s_meidaFileList.totalCount, downloadCount);
        for (int i = 0; i < downloadCount; ++i) {
            if (s_meidaFileList.fileListInfo[i].fileSize < 1 * 1024 * 1024) {
                printf(
                    "\033[1;32;40m ### Media file_%03d name: %s, index: %d, time:%04d-%02d-%02d_%02d:%02d:%02d, size: %.2f KB, type: %d \033[0m\r\n",
                    i, s_meidaFileList.fileListInfo[i].fileName,
                    s_meidaFileList.fileListInfo[i].fileIndex,
                    s_meidaFileList.fileListInfo[i].createTime.year,
                    s_meidaFileList.fileListInfo[i].createTime.month,
                    s_meidaFileList.fileListInfo[i].createTime.day,
                    s_meidaFileList.fileListInfo[i].createTime.hour,
                    s_meidaFileList.fileListInfo[i].createTime.minute,
                    s_meidaFileList.fileListInfo[i].createTime.second,
                    (dji_f32_t) s_meidaFileList.fileListInfo[i].fileSize / 1024,
                    s_meidaFileList.fileListInfo[i].type);
            } else {
                printf(
                    "\033[1;32;40m ### Media file_%03d name: %s, index: %d, time:%04d-%02d-%02d_%02d:%02d:%02d, size: %.2f MB, type: %d \033[0m\r\n",
                    i, s_meidaFileList.fileListInfo[i].fileName,
                    s_meidaFileList.fileListInfo[i].fileIndex,
                    s_meidaFileList.fileListInfo[i].createTime.year,
                    s_meidaFileList.fileListInfo[i].createTime.month,
                    s_meidaFileList.fileListInfo[i].createTime.day,
                    s_meidaFileList.fileListInfo[i].createTime.hour,
                    s_meidaFileList.fileListInfo[i].createTime.minute,
                    s_meidaFileList.fileListInfo[i].createTime.second,
                    (dji_f32_t) s_meidaFileList.fileListInfo[i].fileSize / (1024 * 1024),
                    s_meidaFileList.fileListInfo[i].type);
            }
        }
        printf("\r\n");

        osalHandler->TaskSleepMs(1000);

        if (s_meidaFileList.totalCount < TEST_CAMERA_MANAGER_MEDIA_DOWNLOAD_FILE_NUM) {
            downloadCount = s_meidaFileList.totalCount;
        } else {
            downloadCount = TEST_CAMERA_MANAGER_MEDIA_DOWNLOAD_FILE_NUM;
        }

        for (int i = 0; i < downloadCount; ++i) {
            returnCode = DjiCameraManager_DownloadFileByIndex(position, s_meidaFileList.fileListInfo[i].fileIndex);
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("Download media file by index failed, error code: 0x%08X.", returnCode);
                return returnCode;
            }
        }

        returnCode = DjiCameraManager_DeleteFileByIndex(position, s_meidaFileList.fileListInfo[0].fileIndex);
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("Delete media file by index failed, error code: 0x%08X.", returnCode);
            return returnCode;
        }

        osalHandler->TaskSleepMs(1000);
    } else {
        USER_LOG_WARN("Media file is not existed in sdcard.\r\n");
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTest_CameraManagerDownloadFileDataCallback(T_DjiDownloadFilePacketInfo packetInfo,
                                                                     const uint8_t *data, uint16_t len)
{
    int32_t i;
    float downloadSpeed = 0.0f;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    if (packetInfo.downloadFileEvent == DJI_DOWNLOAD_FILE_EVENT_START) {
        for (i = 0; i < s_meidaFileList.totalCount; ++i) {
            if (s_meidaFileList.fileListInfo[i].fileIndex == packetInfo.fileIndex) {
                break;
            }
        }
        osalHandler->GetTimeMs(&downloadStartMs);

        memset(downloadFileName, 0, sizeof(downloadFileName));
        snprintf(downloadFileName, sizeof(downloadFileName), "%s", s_meidaFileList.fileListInfo[i].fileName);
        USER_LOG_INFO("Start download media file");
        s_downloadMediaFile = fopen(downloadFileName, "wb+");
        if (s_downloadMediaFile == NULL) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
        fwrite(data, 1, len, s_downloadMediaFile);
    } else if (packetInfo.downloadFileEvent == DJI_DOWNLOAD_FILE_EVENT_TRANSFER) {
        if (s_downloadMediaFile != NULL) {
            fwrite(data, 1, len, s_downloadMediaFile);
        }
        printf("\033[1;32;40m ### [Complete rate : %0.1f%%] (%s), size: %d, fileIndex: %d\033[0m\r\n",
               packetInfo.progressInPercent, downloadFileName, packetInfo.fileSize, packetInfo.fileIndex);
        printf("\033[1A");
        USER_LOG_DEBUG("Transfer download media file data, len: %d, percent: %.1f", len, packetInfo.progressInPercent);
    } else if (packetInfo.downloadFileEvent == DJI_DOWNLOAD_FILE_EVENT_END) {
        if (s_downloadMediaFile != NULL) {
            fwrite(data, 1, len, s_downloadMediaFile);
        }
        osalHandler->GetTimeMs(&downloadEndMs);

        downloadSpeed = (float) packetInfo.fileSize / (float) (downloadEndMs - downloadStartMs);
        printf("\033[1;32;40m ### [Complete rate : %0.1f%%] (%s), size: %d, fileIndex: %d\033[0m\r\n",
               packetInfo.progressInPercent, downloadFileName, packetInfo.fileSize, packetInfo.fileIndex);
        printf("\033[1A");
        printf("\r\n");
        USER_LOG_INFO("End download media file, Download Speed %.2f KB/S\r\n\r\n", downloadSpeed);
        fclose(s_downloadMediaFile);
        s_downloadMediaFile = NULL;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/
