#include "handle.h"

namespace uvw {
	template void Emitter<struct FakeHandle, struct CloseEvent, struct ErrorEvent>::Handler<struct ErrorEvent>::publish(struct ErrorEvent, struct FakeHandle&);

	template auto uvw::Resource<struct FakeHandle, struct fake_handle_t, struct uvw::CloseEvent, struct uvw::ErrorEvent>::parent()const noexcept;
	template void uvw::Resource<struct FakeHandle, struct fake_handle_t, struct uvw::CloseEvent, struct uvw::ErrorEvent>::leak() noexcept;
	template bool uvw::Resource<struct FakeHandle, struct fake_handle_t, struct uvw::CloseEvent, struct uvw::ErrorEvent>::self()const noexcept;
}