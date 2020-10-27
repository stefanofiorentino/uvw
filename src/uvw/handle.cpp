#include "handle.h"

namespace uvw {
	// template<> uvw::Emitter<FakeHandle, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
	template void Emitter<FakeHandle, CloseEvent, ErrorEvent>::Handler<ErrorEvent>::publish(struct ErrorEvent, FakeHandle&);

	template auto uvw::Resource<FakeHandle, fake_handle_t, uvw::CloseEvent, uvw::ErrorEvent>::parent()const noexcept;
	template void uvw::Resource<FakeHandle, fake_handle_t, uvw::CloseEvent, uvw::ErrorEvent>::leak() noexcept;
	template bool uvw::Resource<FakeHandle, fake_handle_t, uvw::CloseEvent, uvw::ErrorEvent>::self()const noexcept;
}