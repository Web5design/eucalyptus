/*************************************************************************
 * Copyright 2009-2012 Eucalyptus Systems, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 * Please contact Eucalyptus Systems, Inc., 6755 Hollister Ave., Goleta
 * CA 93117, USA or visit http://www.eucalyptus.com/licenses/ if you need
 * additional information or have any questions.
 *
 * This file may incorporate work covered under the following copyright
 * and permission notice:
 *
 *   Software License Agreement (BSD License)
 *
 *   Copyright (c) 2008, Regents of the University of California
 *   All rights reserved.
 *
 *   Redistribution and use of this software in source and binary forms,
 *   with or without modification, are permitted provided that the
 *   following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE. USERS OF THIS SOFTWARE ACKNOWLEDGE
 *   THE POSSIBLE PRESENCE OF OTHER OPEN SOURCE LICENSED MATERIAL,
 *   COPYRIGHTED MATERIAL OR PATENTED MATERIAL IN THIS SOFTWARE,
 *   AND IF ANY SUCH MATERIAL IS DISCOVERED THE PARTY DISCOVERING
 *   IT MAY INFORM DR. RICH WOLSKI AT THE UNIVERSITY OF CALIFORNIA,
 *   SANTA BARBARA WHO WILL THEN ASCERTAIN THE MOST APPROPRIATE REMEDY,
 *   WHICH IN THE REGENTS' DISCRETION MAY INCLUDE, WITHOUT LIMITATION,
 *   REPLACEMENT OF THE CODE SO IDENTIFIED, LICENSING OF THE CODE SO
 *   IDENTIFIED, OR WITHDRAWAL OF THE CODE CAPABILITY TO THE EXTENT
 *   NEEDED TO COMPLY WITH ANY SUCH LICENSES OR RIGHTS.
 ************************************************************************/

#ifndef SERVER_MARSHAL_H
#define SERVER_MARSHAL_H

#include "axis2_skel_EucalyptusNC.h"

void adb_InitService(void);
adb_ncDescribeResourceResponse_t*  ncDescribeResourceMarshal  (adb_ncDescribeResource_t* ncDescribeResource, const axutil_env_t *env);
adb_ncRunInstanceResponse_t*       ncRunInstanceMarshal       (adb_ncRunInstance_t* ncRunInstance, const axutil_env_t *env);
adb_ncDescribeInstancesResponse_t* ncDescribeInstancesMarshal (adb_ncDescribeInstances_t* ncDescribeInstances, const axutil_env_t *env);
adb_ncTerminateInstanceResponse_t* ncTerminateInstanceMarshal (adb_ncTerminateInstance_t* ncTerminateInstance, const axutil_env_t *env);
adb_ncStartNetworkResponse_t* ncStartNetworkMarshal (adb_ncStartNetwork_t* ncStartNetwork, const axutil_env_t *env);
adb_ncPowerDownResponse_t* ncPowerDownMarshal (adb_ncPowerDown_t* ncPowerDown, const axutil_env_t *env);
adb_ncAssignAddressResponse_t* ncAssignAddressMarshal (adb_ncAssignAddress_t* ncAssignAddress, const axutil_env_t *env);
adb_ncRebootInstanceResponse_t* ncRebootInstanceMarshal (adb_ncRebootInstance_t* ncRebootInstance,  const axutil_env_t *env);
adb_ncGetConsoleOutputResponse_t* ncGetConsoleOutputMarshal (adb_ncGetConsoleOutput_t* ncGetConsoleOutput, const axutil_env_t *env);
adb_ncAttachVolumeResponse_t* ncAttachVolumeMarshal (adb_ncAttachVolume_t* ncAttachVolume, const axutil_env_t *env);
adb_ncDetachVolumeResponse_t* ncDetachVolumeMarshal (adb_ncDetachVolume_t* ncDetachVolume, const axutil_env_t *env);
adb_ncCreateImageResponse_t* ncCreateImageMarshal (adb_ncCreateImage_t* ncCreateImage, const axutil_env_t *env);
adb_ncBundleInstanceResponse_t* ncBundleInstanceMarshal (adb_ncBundleInstance_t* ncBundleInstance, const axutil_env_t *env);
adb_ncCancelBundleTaskResponse_t* ncCancelBundleTaskMarshal (adb_ncCancelBundleTask_t* ncCancelBundleTask, const axutil_env_t *env);
adb_ncDescribeBundleTasksResponse_t* ncDescribeBundleTasksMarshal (adb_ncDescribeBundleTasks_t* ncDescribeBundleTasks, const axutil_env_t *env);

#endif
