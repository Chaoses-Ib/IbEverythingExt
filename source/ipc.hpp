#pragma once
#include <IbEverything/Everything.hpp>

extern ib::Holder<Everythings::EverythingMT> ipc_ev;
extern Everythings::EverythingMT::Version ipc_version;

void ipc_init(std::wstring_view instance_name);
void ipc_destroy();