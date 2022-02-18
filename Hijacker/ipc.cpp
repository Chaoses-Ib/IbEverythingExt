#include "pch.h"
#include "ipc.hpp"
#include "helper.hpp"

ib::Holder<Everythings::EverythingMT> ipc_ev;
Everythings::EverythingMT::Version ipc_version;

void ipc_init(std::wstring_view instance_name) {
    ipc_ev.create(instance_name);
    ipc_version = ipc_ev->get_version();
}

void ipc_destroy() {
    ipc_ev.destroy();
}