//
// kumofs
//
// Copyright (C) 2009 Etolabo Corp.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//
#include "gateway/framework.h"

namespace kumo {
namespace gate {


uint64_t stdhash(const char* key, size_t keylen)
{
	return HashSpace::hash(key, keylen);
}

void fatal_stop()
{
	gateway::net->signal_end();
}


void req_get::submit()
{
	gateway::net->mod_store.Get(*this);
}

void req_set::submit()
{
	gateway::net->mod_store.Set(*this);
}

void req_delete::submit()
{
	gateway::net->mod_store.Delete(*this);
}


}  // namespace gate
}  // namespace kumo

