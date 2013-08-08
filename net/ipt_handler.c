// -*- mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil -*-
// vim: set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <eucalyptus.h>
#include <log.h>
#include <vnetwork.h>

#include "ipt_handler.h"

int ipt_handler_init(ipt_handler *ipth, char *cmdprefix) {
  int fd;
  if (!ipth) {
    return(1);
  }
  bzero(ipth, sizeof(ipt_handler));
  
  snprintf(ipth->ipt_file, MAX_PATH, "/tmp/ipt_file-XXXXXX");
  fd = safe_mkstemp(ipth->ipt_file);
  if (fd < 0) {
    LOGERROR("cannot open ipt_file '%s'\n", ipth->ipt_file);
    return (1);
  }
  chmod(ipth->ipt_file, 0644);
  close(fd);
  
  if (cmdprefix) {
      snprintf(ipth->cmdprefix, MAX_PATH, "%s", cmdprefix);
  } else {
      ipth->cmdprefix[0] = '\0';
  }

  ipth->init = 1;
  return(0);
}

int ipt_system_save(ipt_handler *ipth) {
  int rc, fd;
  char cmd[MAX_PATH];
  
  snprintf(cmd, MAX_PATH, "%s iptables-save > %s", ipth->cmdprefix, ipth->ipt_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute iptables-save (%s)\n", cmd);
  }
  return(rc);
}

int ipt_system_restore(ipt_handler *ipth) {
  int rc;
  char cmd[MAX_PATH];
    
  snprintf(cmd, MAX_PATH, "%s iptables-restore -c < %s", ipth->cmdprefix, ipth->ipt_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute iptables-restore\n");
    snprintf(cmd, MAX_PATH, "cat %s", ipth->ipt_file);
    system(cmd);
  }
  return(rc);
}

int ipt_handler_deploy(ipt_handler *ipth) {
    int i, j, k;
    FILE *FH=NULL;
    if (!ipth || !ipth->init) {
        return(1);
    }
    
    ipt_handler_update_refcounts(ipth);
    
    FH=fopen(ipth->ipt_file, "w");
    if (!FH) {
        LOGERROR("could not open file for write(%s)\n", ipth->ipt_file);
        return(1);
    }
    for (i=0; i<ipth->max_tables; i++) {
        fprintf(FH, "*%s\n", ipth->tables[i].name);
        for (j=0; j<ipth->tables[i].max_chains; j++) {
            if (strcmp(ipth->tables[i].chains[j].name, "EMPTY") && ipth->tables[i].chains[j].ref_count) {
                fprintf(FH, ":%s %s %s\n", ipth->tables[i].chains[j].name, ipth->tables[i].chains[j].policyname, ipth->tables[i].chains[j].counters);
            }
        }
        for (j=0; j<ipth->tables[i].max_chains; j++) {
            if (strcmp(ipth->tables[i].chains[j].name, "EMPTY") && ipth->tables[i].chains[j].ref_count) {
                for (k=0; k<ipth->tables[i].chains[j].max_rules; k++) {
                    fprintf(FH, "%s\n", ipth->tables[i].chains[j].rules[k].iptrule);
                }
            }
        }
        fprintf(FH, "COMMIT\n");
    }
    fclose(FH);
    
    return(ipt_system_restore(ipth));
}

int ipt_handler_repopulate(ipt_handler *ipth) {
  int i, rc;
  FILE *FH=NULL;
  char buf[1024], tmpbuf[1024], *strptr=NULL;
  char tablename[64], chainname[64], policyname[64], counters[64];

  if (!ipth || !ipth->init) {
    return(1);
  }
  
  rc = ipt_handler_free(ipth);
  if (rc) {
    return(1);
  }

  rc = ipt_system_save(ipth);
  if (rc) {
    LOGERROR("could not save current IPT rules to file, skipping re-populate\n");
    return(1);
  }
  
  FH=fopen(ipth->ipt_file, "r");
  if (!FH) {
    LOGERROR("could not open file for read(%s)\n", ipth->ipt_file);
    return(1);
  }
    
  while (fgets(buf, 1024, FH)) {
    if ( (strptr = strchr(buf, '\n')) ) {
      *strptr = '\0';
    }

    if (strlen(buf) < 1) {
      continue;
    }
    
    while(buf[strlen(buf)-1] == ' ') {
      buf[strlen(buf)-1] = '\0';
    }

    if (buf[0] == '*') {
      tablename[0] = '\0';
      sscanf(buf, "%[*]%s", tmpbuf, tablename);
      if (strlen(tablename)) {
	ipt_handler_add_table(ipth, tablename);
      }
    } else if (buf[0] == ':') {
      chainname[0] = '\0';
      sscanf(buf, "%[:]%s %s %s", tmpbuf, chainname, policyname, counters);
      if (strlen(chainname)) {
	ipt_table_add_chain(ipth, tablename, chainname, policyname, counters);
      }
    } else if (strstr(buf, "COMMIT")) {
    } else if (buf[0] == '#') {
    } else if (buf[0] == '-' && buf[1] == 'A') {
      sscanf(buf, "%[-A] %s", tmpbuf, chainname);
      ipt_chain_add_rule(ipth, tablename, chainname, buf);
    } else {
      LOGWARN("unknown IPT rule on ingress, will be thrown out: (%s)\n", buf);
    }
  }
  fclose(FH);
  
  return(0);
}

int ipt_handler_add_table(ipt_handler *ipth, char *tablename) {
  ipt_table *table=NULL;
  if (!ipth || !tablename || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    ipth->tables = realloc(ipth->tables, sizeof(ipt_table) * (ipth->max_tables+1));
    bzero(&(ipth->tables[ipth->max_tables]), sizeof(ipt_table));
    snprintf(ipth->tables[ipth->max_tables].name, 64, tablename);
    ipth->max_tables++;
  }
  
  return(0);
}
int ipt_table_add_chain(ipt_handler *ipth, char *tablename, char *chainname, char *policyname, char *counters) {
  ipt_table *table=NULL;
  ipt_chain *chain=NULL;
  if (!ipth || !tablename || !chainname || !counters || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(1);
  }
  
  chain = ipt_table_find_chain(ipth, tablename, chainname);
  if (!chain) {
    table->chains = realloc(table->chains, sizeof(ipt_chain) * (table->max_chains+1));
    bzero(&(table->chains[table->max_chains]), sizeof(ipt_chain));
    snprintf(table->chains[table->max_chains].name, 64, "%s", chainname);
    snprintf(table->chains[table->max_chains].policyname, 64, "%s", policyname);
    snprintf(table->chains[table->max_chains].counters, 64, "%s", counters);
    if (!strcmp(table->chains[table->max_chains].name, "INPUT") ||
	!strcmp(table->chains[table->max_chains].name, "FORWARD") ||
	!strcmp(table->chains[table->max_chains].name, "OUTPUT") ||
	!strcmp(table->chains[table->max_chains].name, "PREROUTING") ||
	!strcmp(table->chains[table->max_chains].name, "POSTROUTING")) {
      table->chains[table->max_chains].ref_count=1;
    }
	
    table->max_chains++;

  }

  return(0);
}

int ipt_chain_add_rule(ipt_handler *ipth, char *tablename, char *chainname, char *newrule) {
  ipt_table *table=NULL;
  ipt_chain *chain=NULL;
  ipt_rule *rule=NULL;

  if (!ipth || !tablename || !chainname || !newrule || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(1);
  }

  chain = ipt_table_find_chain(ipth, tablename, chainname);
  if (!chain) {
    return(1);
  }
  
  rule = ipt_chain_find_rule(ipth, tablename, chainname, newrule);
  if (!rule) {
    chain->rules = realloc(chain->rules, sizeof(ipt_rule) * (chain->max_rules+1));
    bzero(&(chain->rules[chain->max_rules]), sizeof(ipt_rule));
    snprintf(chain->rules[chain->max_rules].iptrule, 1024, "%s", newrule);
    chain->max_rules++;
  }
  return(0);
}

int ipt_handler_update_refcounts(ipt_handler *ipth) {
    char *jumpptr=NULL, jumpchain[64], tmp[64];
    int i, j, k;
    ipt_table *table=NULL;
    ipt_chain *chain=NULL, *refchain=NULL;
    ipt_rule *rule=NULL;
    
    for (i=0; i<ipth->max_tables; i++) {
        table = &(ipth->tables[i]);
        for (j=0; j<table->max_chains; j++) {
            chain = &(table->chains[j]);
            for (k=0; k<chain->max_rules; k++) {
                rule = &(chain->rules[k]);

                jumpptr = strstr(rule->iptrule, "-j");
                if (jumpptr) {
                    jumpchain[0] = '\0';
                    sscanf(jumpptr, "%[-j] %s", tmp, jumpchain);
                    if (strlen(jumpchain)) {
                        refchain = ipt_table_find_chain(ipth, table->name, jumpchain);
                        if (refchain) {
                            LOGDEBUG("FOUND REF TO CHAIN (name=%s sourcechain=%s jumpchain=%s currref=%d) (rule=%s\n", refchain->name, chain->name, jumpchain, refchain->ref_count, rule->iptrule);
                            refchain->ref_count++;
                        }
                    }
                }
            }
        }
    }
    return(0);
}

ipt_table *ipt_handler_find_table(ipt_handler *ipth, char *findtable) {
  int i, tableidx=0, found=0;
  if (!ipth || !findtable || !ipth->init) {
    return(NULL);
  }
  
  found=0;
  for (i=0; i<ipth->max_tables && !found; i++) {
    tableidx=i;
    if (!strcmp(ipth->tables[i].name, findtable)) found++;
  }
  if (!found) {
    return(NULL);
  }
  return(&(ipth->tables[tableidx]));  
}

ipt_chain *ipt_table_find_chain(ipt_handler *ipth, char *tablename, char *findchain) {
  int i, found=0, chainidx=0;
  ipt_table *table=NULL;

  if (!ipth || !tablename || !findchain || !ipth->init) {
    return(NULL);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(NULL);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    chainidx=i;
    if (!strcmp(table->chains[i].name, findchain)) found++;
  }
  
  if (!found) {
    return(NULL);
  }
  
  return(&(table->chains[chainidx]));
}

ipt_rule *ipt_chain_find_rule(ipt_handler *ipth, char *tablename, char *chainname, char *findrule) {
  int i, found=0, ruleidx=0;
  ipt_chain *chain;

  if (!ipth || !tablename || !chainname || !findrule || !ipth->init) {
    return(NULL);
  }

  chain = ipt_table_find_chain(ipth, tablename, chainname);
  if (!chain) {
    return(NULL);
  }
  
  for (i=0; i<chain->max_rules; i++) {
    ruleidx=i;
    if (!strcmp(chain->rules[i].iptrule, findrule)) found++;
  }
  if (!found) {
    return(NULL);
  }
  return(&(chain->rules[i]));
}

int ipt_table_deletechainempty(ipt_handler *ipth, char *tablename) {
  int i, found=0;
  ipt_table *table=NULL;

  if (!ipth || !tablename || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(1);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    if (table->chains[i].max_rules == 0) {
      ipt_table_deletechainmatch(ipth, tablename, table->chains[i].name);
      found++;
    }
  }
  if (!found) {
    return(1);
  }
  return(0);
}

int ipt_table_deletechainmatch(ipt_handler *ipth, char *tablename, char *chainmatch) {
  int i, found=0;
  ipt_table *table=NULL;

  if (!ipth || !tablename || !chainmatch || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(1);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    if (strstr(table->chains[i].name, chainmatch)) {
      EUCA_FREE(table->chains[i].rules);
      bzero(&(table->chains[i]), sizeof(ipt_chain));
      snprintf(table->chains[i].name, 64, "EMPTY");
    }
  }

  return(0);
}

int ipt_chain_flush(ipt_handler *ipth, char *tablename, char *chainname) {
  ipt_table *table=NULL;
  ipt_chain *chain=NULL;

  if (!ipth || !tablename || !chainname || !ipth->init) {
    return(1);
  }
  
  table = ipt_handler_find_table(ipth, tablename);
  if (!table) {
    return(1);
  }
  chain = ipt_table_find_chain(ipth, tablename, chainname);
  if (!chain) {
    return(1);
  }
  
  EUCA_FREE(chain->rules);
  chain->max_rules = 0;
  snprintf(chain->counters, 64, "[0:0]");

  return(0);
}

int ipt_handler_free(ipt_handler *ipth) {
  int i, j, k;
  char saved_cmdprefix[MAX_PATH];
  if (!ipth || !ipth->init) {
    return(1);
  }
  snprintf(saved_cmdprefix, MAX_PATH, "%s", ipth->cmdprefix);

  for (i=0; i<ipth->max_tables; i++) {
    for (j=0; j<ipth->tables[i].max_chains; j++) {
      EUCA_FREE(ipth->tables[i].chains[j].rules);
    }
    EUCA_FREE(ipth->tables[i].chains);
  }
  EUCA_FREE(ipth->tables);
  unlink(ipth->ipt_file);

  return(ipt_handler_init(ipth, saved_cmdprefix));
}

int ipt_handler_print(ipt_handler *ipth) {
  int i, j, k;
  if (!ipth || !ipth->init) {
    return(1);
  }
  
  for (i=0; i<ipth->max_tables; i++) {
    LOGDEBUG("TABLE (%d of %d): %s\n", i, ipth->max_tables, ipth->tables[i].name);
    for (j=0; j<ipth->tables[i].max_chains; j++) {
      LOGDEBUG("\tCHAIN: (%d of %d, refcount=%d): %s %s %s\n", j, ipth->tables[i].max_chains, ipth->tables[i].chains[j].ref_count, ipth->tables[i].chains[j].name, ipth->tables[i].chains[j].policyname, ipth->tables[i].chains[j].counters);
      for (k=0; k<ipth->tables[i].chains[j].max_rules; k++) {
	LOGDEBUG("\t\tRULE (%d of %d): %s\n", k, ipth->tables[i].chains[j].max_rules, ipth->tables[i].chains[j].rules[k].iptrule);
      }
    }
  }

  return(0);
}

/**************/

int ips_handler_init(ips_handler *ipsh, char *cmdprefix) {
  int fd;
  if (!ipsh) {
    LOGERROR("null passed to ips_handler_init()\n");
    return(1);
  }
  bzero(ipsh, sizeof(ips_handler));
  
  snprintf(ipsh->ips_file, MAX_PATH, "/tmp/ips_file-XXXXXX");
  fd = safe_mkstemp(ipsh->ips_file);
  if (fd < 0) {
    LOGERROR("cannot open ips_file '%s'\n", ipsh->ips_file);
    return (1);
  }
  chmod(ipsh->ips_file, 0644);
  close(fd);
  
  if (cmdprefix) {
      snprintf(ipsh->cmdprefix, MAX_PATH, "%s", cmdprefix);
  } else {
      ipsh->cmdprefix[0] = '\0';
  }
  ipsh->init = 1;
  return(0);
}

int ips_system_save(ips_handler *ipsh) {
  int rc, fd;
  char cmd[MAX_PATH];
  
  snprintf(cmd, MAX_PATH, "%s ipset save > %s", ipsh->cmdprefix, ipsh->ips_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute ipset save (%s)\n", cmd);
  }
  return(rc);
}
int ips_system_restore(ips_handler *ipsh) {
    int rc;
  char cmd[MAX_PATH];
    
  snprintf(cmd, MAX_PATH, "%s ipset -! restore < %s", ipsh->cmdprefix, ipsh->ips_file);
  rc = system(cmd);
  rc = rc>>8;
  LOGDEBUG("RESTORE CMD: %s\n", cmd);
  if (rc) {
    LOGERROR("failed to execute ipset restore (%s)\n", cmd);
    snprintf(cmd, MAX_PATH, "cat %s", ipsh->ips_file);
    system(cmd);
  }
  snprintf(cmd, MAX_PATH, "cat %s", ipsh->ips_file);
  system(cmd);
  return(rc);
}

int ips_handler_repopulate(ips_handler *ipsh) {
  int i, rc;
  FILE *FH=NULL;
  char buf[1024], tmpbuf[1024], *strptr=NULL;
  char setname[64], ipname[64];

  if (!ipsh || !ipsh->init) {
    return(1);
  }
  
  rc = ips_handler_free(ipsh);
  if (rc) {
    return(1);
  }

  rc = ips_system_save(ipsh);
  if (rc) {
    LOGERROR("could not save current IPS rules to file, skipping re-populate\n");
    return(1);
  }
  
  FH=fopen(ipsh->ips_file, "r");
  if (!FH) {
    LOGERROR("could not open file for read(%s)\n", ipsh->ips_file);
    return(1);
  }
    
  while (fgets(buf, 1024, FH)) {
    if ( (strptr = strchr(buf, '\n')) ) {
      *strptr = '\0';
    }

    if (strlen(buf) < 1) {
      continue;
    }
    
    while(buf[strlen(buf)-1] == ' ') {
      buf[strlen(buf)-1] = '\0';
    }

    if (strstr(buf, "create")) {
        setname[0] = '\0';
        sscanf(buf, "create %s", setname);
        if (strlen(setname)) {
            ips_handler_add_set(ipsh, setname);
        }
    } else if (strstr(buf, "add")) {
        ipname[0] = '\0';
        sscanf(buf, "add %s %[0-9.]", setname, ipname);
        if (strlen(setname) && strlen(ipname)) {
            ips_set_add_ip(ipsh, setname, ipname);
        }
    } else {
        LOGWARN("unknown IPS rule on ingress, will be thrown out: (%s)\n", buf);
    }
  }
  fclose(FH);
  
  return(0);    
}

int ips_handler_deploy(ips_handler *ipsh, int dodelete) {
  int i, j, k;
  FILE *FH=NULL;
  char *strptra=NULL;

  if (!ipsh || !ipsh->init) {
    return(1);
  }
  
  //  ips_handler_update_refcounts(ipsh);

  FH=fopen(ipsh->ips_file, "w");
  if (!FH) {
      LOGERROR("could not open file for write(%s)\n", ipsh->ips_file);
      return(1);
  }
  for (i=0; i<ipsh->max_sets; i++) {
      if (ipsh->sets[i].ref_count) {
          fprintf(FH, "create %s hash:ip family inet hashsize 2048 maxelem 65536\n", ipsh->sets[i].name);
          fprintf(FH, "flush %s\n", ipsh->sets[i].name);
          for (j=0; j<ipsh->sets[i].max_member_ips; j++) {
              strptra = hex2dot(ipsh->sets[i].member_ips[j]);
              fprintf(FH, "add %s %s\n", ipsh->sets[i].name, strptra);
              EUCA_FREE(strptra);
          }
      } else if ( (ipsh->sets[i].ref_count == 0) && dodelete) {
          fprintf(FH, "flush %s\n", ipsh->sets[i].name);
          fprintf(FH, "destroy %s\n", ipsh->sets[i].name);
      }
  }
  fclose(FH);
  
  return(ips_system_restore(ipsh));
}

int ips_handler_add_set(ips_handler *ipsh, char *setname) {
  ips_set *set=NULL;
    
  if (!ipsh || !setname || !ipsh->init) {
    return(1);
  }
  
  set = ips_handler_find_set(ipsh, setname);
  if (!set) {
    ipsh->sets = realloc(ipsh->sets, sizeof(ips_set) * (ipsh->max_sets+1));
    bzero(&(ipsh->sets[ipsh->max_sets]), sizeof(ips_set));
    snprintf(ipsh->sets[ipsh->max_sets].name, 64, setname);
    ipsh->max_sets++;
  }
  return(0);
}

ips_set *ips_handler_find_set(ips_handler *ipsh, char *findset) {
  int i, setidx=0, found=0;
  if (!ipsh || !findset || !ipsh->init) {
    return(NULL);
  }
  
  found=0;
  for (i=0; i<ipsh->max_sets && !found; i++) {
    setidx=i;
    if (!strcmp(ipsh->sets[i].name, findset)) found++;
  }
  if (!found) {
    return(NULL);
  }
  return(&(ipsh->sets[setidx]));  
}

int ips_set_add_ip(ips_handler *ipsh, char *setname, char *ipname) {
  ips_set *set=NULL;
  u32 *ip=NULL;
  if (!ipsh || !setname || !ipname || !ipsh->init) {
    return(1);
  }
  
  set = ips_handler_find_set(ipsh, setname);
  if (!set) {
    return(1);
  }
  
  ip = ips_set_find_ip(ipsh, setname, ipname);
  if (!ip) {
    set->member_ips = realloc(set->member_ips, sizeof(u32) * (set->max_member_ips+1));
    bzero(&(set->member_ips[set->max_member_ips]), sizeof(u32));
    set->member_ips[set->max_member_ips] = dot2hex(ipname);
    set->max_member_ips++;
    set->ref_count++;
  }
  return(0);
}

u32 *ips_set_find_ip(ips_handler *ipsh, char *setname, char *findipstr) {
  int i, found=0, ipidx=0;
  ips_set *set=NULL;
  u32 findip;

  if (!ipsh || !setname || !findipstr || !ipsh->init) {
    return(NULL);
  }
  
  set = ips_handler_find_set(ipsh, setname);
  if (!set) {
    return(NULL);
  }
  
  findip = dot2hex(findipstr);
  found=0;
  for (i=0; i<set->max_member_ips && !found; i++) {
    ipidx=i;
    if (set->member_ips[i] == findip) found++;
  }

  if (!found) {
    return(NULL);
  }
  
  return(&(set->member_ips[ipidx]));
    return(0);
}

int ips_set_flush(ips_handler *ipsh, char *setname) {
  ips_set *set=NULL;

  if (!ipsh || !setname || !ipsh->init) {
    return(1);
  }
  
  set = ips_handler_find_set(ipsh, setname);
  if (!set) {
    return(1);
  }
  
  EUCA_FREE(set->member_ips);
  set->max_member_ips = set->ref_count = 0;

  return(0);
}

int ips_handler_deletesetmatch(ips_handler *ipsh, char *setmatch) {
  int i, found=0;
  ips_set *set=NULL;

  if (!ipsh || !setmatch || !ipsh->init) {
    return(1);
  }
  
  found=0;
  for (i=0; i<ipsh->max_sets && !found; i++) {
      if (strstr(ipsh->sets[i].name, setmatch)) {
          EUCA_FREE(ipsh->sets[i].member_ips);
          //          bzero(&(ipsh->sets[i]), sizeof(ips_set));
          ipsh->sets[i].max_member_ips = 0;
          ipsh->sets[i].ref_count = 0;
      }
  }
  
  return(0);
}

int ips_handler_free(ips_handler *ipsh) {
  int i, j, k;
  char saved_cmdprefix[MAX_PATH];
  if (!ipsh || !ipsh->init) {
    return(1);
  }
  snprintf(saved_cmdprefix, MAX_PATH, "%s", ipsh->cmdprefix);

  for (i=0; i<ipsh->max_sets; i++) {
      EUCA_FREE(ipsh->sets[i].member_ips);
  }
  EUCA_FREE(ipsh->sets);

  unlink(ipsh->ips_file);

  return(ips_handler_init(ipsh, saved_cmdprefix));
}

int ips_handler_print(ips_handler *ipsh) {
    int i, j;
    char *strptra=NULL;

    for (i=0; i<ipsh->max_sets; i++) {
        LOGDEBUG("IPSET NAME: %s\n", ipsh->sets[i].name);
        for (j=0; j<ipsh->sets[i].max_member_ips; j++) {
            strptra = hex2dot(ipsh->sets[i].member_ips[j]);
            LOGDEBUG("\t MEMBER IP: %s\n", strptra);
            EUCA_FREE(strptra);
        }
    }
    return(0);
}

/************EBT*****************/

int ebt_handler_init(ebt_handler *ebth, char *cmdprefix) {
  int fd;
  if (!ebth) {
    return(1);
  }
  bzero(ebth, sizeof(ebt_handler));
  
  snprintf(ebth->ebt_file, MAX_PATH, "/tmp/ebt_file-XXXXXX");
  fd = safe_mkstemp(ebth->ebt_file);
  if (fd < 0) {
    LOGERROR("cannot open ebt_file '%s'\n", ebth->ebt_file);
    return (1);
  }
  chmod(ebth->ebt_file, 0644);
  close(fd);

  snprintf(ebth->ebt_asc_file, MAX_PATH, "/tmp/ebt_asc_file-XXXXXX");
  fd = safe_mkstemp(ebth->ebt_asc_file);
  if (fd < 0) {
    LOGERROR("cannot open ebt_asc_file '%s'\n", ebth->ebt_asc_file);
    unlink(ebth->ebt_file);
    return (1);
  }
  chmod(ebth->ebt_file, 0644);
  close(fd);
  
  if (cmdprefix) {
      snprintf(ebth->cmdprefix, MAX_PATH, "%s", cmdprefix);
  } else {
      ebth->cmdprefix[0] = '\0';
  }

  ebth->init = 1;
  return(0);
}

int ebt_system_save(ebt_handler *ebth) {
  int rc, ret, fd;
  char cmd[MAX_PATH];
  
  ret=0;
  snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s --atomic-save", ebth->cmdprefix, ebth->ebt_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute ebtables-save (%s)\n", cmd);
    ret=1;
  }

  snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s -L > %s", ebth->cmdprefix, ebth->ebt_file, ebth->ebt_asc_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute ebtables list (%s)\n", cmd);
    ret=1;
  }

  return(ret);
}

int ebt_system_restore(ebt_handler *ebth) {
  int rc;
  char cmd[MAX_PATH];
    
  snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s --atomic-commit", ebth->cmdprefix, ebth->ebt_file);
  rc = system(cmd);
  rc = rc>>8;
  if (rc) {
    LOGERROR("failed to execute ebtables-restore\n");
    snprintf(cmd, MAX_PATH, "ebtables --atomic-file %s -L", ebth->ebt_file);
    system(cmd);
  }
  return(rc);
}

int ebt_handler_deploy(ebt_handler *ebth) {
    int i, j, k, rc;
    char cmd[MAX_PATH];
    FILE *FH=NULL;

    if (!ebth || !ebth->init) {
        return(1);
    }

    ebt_handler_update_refcounts(ebth);

    snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s --atomic-init", ebth->cmdprefix, ebth->ebt_file);
    rc = system(cmd);
    rc = rc>>8;
    if (rc) {
        LOGERROR("failed to execute ebtables-save (%s)\n", cmd);
        return(1);
    }
    
    for (i=0; i<ebth->max_tables; i++) {
        for (j=0; j<ebth->tables[i].max_chains; j++) {
            if (strcmp(ebth->tables[i].chains[j].name, "EMPTY") && ebth->tables[i].chains[j].ref_count) {
                if (strcmp(ebth->tables[i].chains[j].name, "INPUT") && strcmp(ebth->tables[i].chains[j].name, "OUTPUT") && strcmp(ebth->tables[i].chains[j].name, "FORWARD")) {
                    snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s -N %s", ebth->cmdprefix, ebth->ebt_file, ebth->tables[i].chains[j].name);
                    system(cmd);
                }
            }
        }
        for (j=0; j<ebth->tables[i].max_chains; j++) {
            if (strcmp(ebth->tables[i].chains[j].name, "EMPTY") && ebth->tables[i].chains[j].ref_count) {
                for (k=0; k<ebth->tables[i].chains[j].max_rules; k++) {
                    snprintf(cmd, MAX_PATH, "%s ebtables --atomic-file %s -A %s %s", ebth->cmdprefix, ebth->ebt_file, ebth->tables[i].chains[j].name, ebth->tables[i].chains[j].rules[k].ebtrule);
                    system(cmd);
                }
            }
        }
    }
    return(ebt_system_restore(ebth));
}

int ebt_handler_repopulate(ebt_handler *ebth) {
  int i, rc;
  FILE *FH=NULL;
  char buf[1024], tmpbuf[1024], *strptr=NULL;
  char tablename[64], chainname[64], policyname[64], counters[64];

  if (!ebth || !ebth->init) {
    return(1);
  }
  
  rc = ebt_handler_free(ebth);
  if (rc) {
    return(1);
  }

  rc = ebt_system_save(ebth);
  if (rc) {
    LOGERROR("could not save current EBT rules to file, skipping re-populate\n");
    return(1);
  }
  
  FH=fopen(ebth->ebt_asc_file, "r");
  if (!FH) {
    LOGERROR("could not open file for read(%s)\n", ebth->ebt_file);
    return(1);
  }
    
  while (fgets(buf, 1024, FH)) {
    if ( (strptr = strchr(buf, '\n')) ) {
      *strptr = '\0';
    }

    if (strlen(buf) < 1) {
      continue;
    }
    
    while(buf[strlen(buf)-1] == ' ') {
      buf[strlen(buf)-1] = '\0';
    }

    if (strstr(buf, "Bridge table:")) {
      tablename[0] = '\0';
      sscanf(buf, "Bridge table: %s", tablename);
      if (strlen(tablename)) {
          ebt_handler_add_table(ebth, tablename);
      }
    } else if (strstr(buf, "Bridge chain: ")) {
      chainname[0] = '\0';
      sscanf(buf, "Bridge chain: %[^,]%s %s %s %s %s", chainname, tmpbuf, tmpbuf, tmpbuf, tmpbuf, policyname);
      if (strlen(chainname)) {
          ebt_table_add_chain(ebth, tablename, chainname, policyname, "");
      }
    } else if (buf[0] == '#') {
    } else if (buf[0] == '-') {
      ebt_chain_add_rule(ebth, tablename, chainname, buf);
    } else {
      LOGWARN("unknown EBT rule on ingress, will be thrown out: (%s)\n", buf);
    }
  }
  fclose(FH);
  
  return(0);
}

int ebt_handler_add_table(ebt_handler *ebth, char *tablename) {
  ebt_table *table=NULL;
  if (!ebth || !tablename || !ebth->init) {
    return(1);
  }

  LOGDEBUG("adding table %s\n", tablename);
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    ebth->tables = realloc(ebth->tables, sizeof(ebt_table) * (ebth->max_tables+1));
    bzero(&(ebth->tables[ebth->max_tables]), sizeof(ebt_table));
    snprintf(ebth->tables[ebth->max_tables].name, 64, tablename);
    ebth->max_tables++;
  }
  
  return(0);
}
int ebt_table_add_chain(ebt_handler *ebth, char *tablename, char *chainname, char *policyname, char *counters) {
  ebt_table *table=NULL;
  ebt_chain *chain=NULL;
  if (!ebth || !tablename || !chainname || !counters || !ebth->init) {
    return(1);
  }
  LOGDEBUG("adding chain %s to table %s\n", chainname, tablename);
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(1);
  }
  
  chain = ebt_table_find_chain(ebth, tablename, chainname);
  if (!chain) {
    table->chains = realloc(table->chains, sizeof(ebt_chain) * (table->max_chains+1));
    bzero(&(table->chains[table->max_chains]), sizeof(ebt_chain));
    snprintf(table->chains[table->max_chains].name, 64, "%s", chainname);
    snprintf(table->chains[table->max_chains].policyname, 64, "%s", policyname);
    snprintf(table->chains[table->max_chains].counters, 64, "%s", counters);
    if (!strcmp(table->chains[table->max_chains].name, "INPUT") ||
	!strcmp(table->chains[table->max_chains].name, "FORWARD") ||
	!strcmp(table->chains[table->max_chains].name, "OUTPUT") ||
	!strcmp(table->chains[table->max_chains].name, "PREROUTING") ||
	!strcmp(table->chains[table->max_chains].name, "POSTROUTING")) {
      table->chains[table->max_chains].ref_count=1;
    }
	
    table->max_chains++;

  }

  return(0);
}

int ebt_chain_add_rule(ebt_handler *ebth, char *tablename, char *chainname, char *newrule) {
  ebt_table *table=NULL;
  ebt_chain *chain=NULL;
  ebt_rule *rule=NULL;

  LOGDEBUG("adding rules (%s) to chain %s to table %s\n", newrule, chainname, tablename);
  if (!ebth || !tablename || !chainname || !newrule || !ebth->init) {
    return(1);
  }
  
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(1);
  }

  chain = ebt_table_find_chain(ebth, tablename, chainname);
  if (!chain) {
    return(1);
  }
  
  rule = ebt_chain_find_rule(ebth, tablename, chainname, newrule);
  if (!rule) {
    chain->rules = realloc(chain->rules, sizeof(ebt_rule) * (chain->max_rules+1));
    bzero(&(chain->rules[chain->max_rules]), sizeof(ebt_rule));
    snprintf(chain->rules[chain->max_rules].ebtrule, 1024, "%s", newrule);
    chain->max_rules++;
  }
  return(0);
}

int ebt_handler_update_refcounts(ebt_handler *ebth) {
    char *jumpptr=NULL, jumpchain[64], tmp[64];
    int i, j, k;
    ebt_table *table=NULL;
    ebt_chain *chain=NULL, *refchain=NULL;
    ebt_rule *rule=NULL;
    
    for (i=0; i<ebth->max_tables; i++) {
        table = &(ebth->tables[i]);
        for (j=0; j<table->max_chains; j++) {
            chain = &(table->chains[j]);
            for (k=0; k<chain->max_rules; k++) {
                rule = &(chain->rules[k]);

                jumpptr = strstr(rule->ebtrule, "-j");
                if (jumpptr) {
                    jumpchain[0] = '\0';
                    sscanf(jumpptr, "%[-j] %s", tmp, jumpchain);
                    if (strlen(jumpchain)) {
                        refchain = ebt_table_find_chain(ebth, table->name, jumpchain);
                        if (refchain) {
                            LOGDEBUG("FOUND REF TO CHAIN (name=%s sourcechain=%s jumpchain=%s currref=%d) (rule=%s\n", refchain->name, chain->name, jumpchain, refchain->ref_count, rule->ebtrule);
                            refchain->ref_count++;
                        }
                    }
                }
            }
        }
    }
    return(0);
}

ebt_table *ebt_handler_find_table(ebt_handler *ebth, char *findtable) {
  int i, tableidx=0, found=0;
  if (!ebth || !findtable || !ebth->init) {
    return(NULL);
  }
  
  found=0;
  for (i=0; i<ebth->max_tables && !found; i++) {
    tableidx=i;
    if (!strcmp(ebth->tables[i].name, findtable)) found++;
  }
  if (!found) {
    return(NULL);
  }
  return(&(ebth->tables[tableidx]));  
}

ebt_chain *ebt_table_find_chain(ebt_handler *ebth, char *tablename, char *findchain) {
  int i, found=0, chainidx=0;
  ebt_table *table=NULL;

  if (!ebth || !tablename || !findchain || !ebth->init) {
    return(NULL);
  }
  
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(NULL);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    chainidx=i;
    if (!strcmp(table->chains[i].name, findchain)) found++;
  }
  
  if (!found) {
    return(NULL);
  }
  
  return(&(table->chains[chainidx]));
}

ebt_rule *ebt_chain_find_rule(ebt_handler *ebth, char *tablename, char *chainname, char *findrule) {
  int i, found=0, ruleidx=0;
  ebt_chain *chain;

  if (!ebth || !tablename || !chainname || !findrule || !ebth->init) {
    return(NULL);
  }

  chain = ebt_table_find_chain(ebth, tablename, chainname);
  if (!chain) {
    return(NULL);
  }
  
  for (i=0; i<chain->max_rules; i++) {
    ruleidx=i;
    if (!strcmp(chain->rules[i].ebtrule, findrule)) found++;
  }
  if (!found) {
    return(NULL);
  }
  return(&(chain->rules[i]));
}

int ebt_table_deletechainempty(ebt_handler *ebth, char *tablename) {
  int i, found=0;
  ebt_table *table=NULL;

  if (!ebth || !tablename || !ebth->init) {
    return(1);
  }
  
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(1);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    if (table->chains[i].max_rules == 0) {
      ebt_table_deletechainmatch(ebth, tablename, table->chains[i].name);
      found++;
    }
  }
  if (!found) {
    return(1);
  }
  return(0);
}

int ebt_table_deletechainmatch(ebt_handler *ebth, char *tablename, char *chainmatch) {
  int i, found=0;
  ebt_table *table=NULL;

  if (!ebth || !tablename || !chainmatch || !ebth->init) {
    return(1);
  }
  
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(1);
  }
  
  found=0;
  for (i=0; i<table->max_chains && !found; i++) {
    if (strstr(table->chains[i].name, chainmatch)) {
      EUCA_FREE(table->chains[i].rules);
      bzero(&(table->chains[i]), sizeof(ebt_chain));
      snprintf(table->chains[i].name, 64, "EMPTY");
    }
  }

  return(0);
}

int ebt_chain_flush(ebt_handler *ebth, char *tablename, char *chainname) {
  ebt_table *table=NULL;
  ebt_chain *chain=NULL;

  if (!ebth || !tablename || !chainname || !ebth->init) {
    return(1);
  }
  
  table = ebt_handler_find_table(ebth, tablename);
  if (!table) {
    return(1);
  }
  chain = ebt_table_find_chain(ebth, tablename, chainname);
  if (!chain) {
    return(1);
  }
  
  EUCA_FREE(chain->rules);
  chain->max_rules = 0;
  chain->counters[0] = '\0';

  return(0);
}

int ebt_handler_free(ebt_handler *ebth) {
  int i, j, k;
  char saved_cmdprefix[MAX_PATH];
  if (!ebth || !ebth->init) {
    return(1);
  }
  snprintf(saved_cmdprefix, MAX_PATH, "%s", ebth->cmdprefix);

  for (i=0; i<ebth->max_tables; i++) {
    for (j=0; j<ebth->tables[i].max_chains; j++) {
      EUCA_FREE(ebth->tables[i].chains[j].rules);
    }
    EUCA_FREE(ebth->tables[i].chains);
  }
  EUCA_FREE(ebth->tables);
  unlink(ebth->ebt_file);
  unlink(ebth->ebt_asc_file);

  return(ebt_handler_init(ebth, saved_cmdprefix));
}

int ebt_handler_print(ebt_handler *ebth) {
  int i, j, k;
  if (!ebth || !ebth->init) {
    return(1);
  }
  
  for (i=0; i<ebth->max_tables; i++) {
    LOGDEBUG("TABLE (%d of %d): %s\n", i, ebth->max_tables, ebth->tables[i].name);
    for (j=0; j<ebth->tables[i].max_chains; j++) {
      LOGDEBUG("\tCHAIN: (%d of %d, refcount=%d): %s policy=%s counters=%s\n", j, ebth->tables[i].max_chains, ebth->tables[i].chains[j].ref_count, ebth->tables[i].chains[j].name, ebth->tables[i].chains[j].policyname, ebth->tables[i].chains[j].counters);
      for (k=0; k<ebth->tables[i].chains[j].max_rules; k++) {
	LOGDEBUG("\t\tRULE (%d of %d): %s\n", k, ebth->tables[i].chains[j].max_rules, ebth->tables[i].chains[j].rules[k].ebtrule);
      }
    }
  }

  return(0);
}

