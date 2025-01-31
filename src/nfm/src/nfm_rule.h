/*
* Copyright (c) 2019, Sagemcom.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NFM_RULE_H_INCLUDED
#define NFM_RULE_H_INCLUDED

#include "ds_tree.h"
#include "schema.h"
#include <stdbool.h>

#define NFM_FLAG_RULE_IN_TREE (1 << 0)
#define NFM_FLAG_RULE_CHAIN4_REFERENCED (1 << 1)
#define NFM_FLAG_RULE_CHAIN6_REFERENCED (1 << 2)
#define NFM_FLAG_RULE_CHAIN_ETH_REFERENCED (1 << 3)
#define NFM_FLAG_RULE_TARGET4_REFERENCED (1 << 4)
#define NFM_FLAG_RULE_TARGET6_REFERENCED (1 << 5)
#define NFM_FLAG_RULE_TARGET_ETH_REFERENCED (1 << 6)
#define NFM_FLAG_RULE_APPLIED (1 << 7)

struct nfm_rule {
	struct ds_tree_node elt;
	struct schema_Netfilter conf;
	uint8_t flags;
};

bool nfm_rule_init(void);
bool nfm_rule_new(const struct schema_Netfilter *conf);
bool nfm_rule_del(const struct schema_Netfilter *conf);
bool nfm_rule_modify(const struct schema_Netfilter *conf);
struct nfm_rule *nfm_rule_get(const char *name);


#endif /* NFM_RULE_H_INCLUDED */
