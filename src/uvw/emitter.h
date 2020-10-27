#ifndef UVW_EMITTER_INCLUDE_H
#define UVW_EMITTER_INCLUDE_H


#include <type_traits>
#include <functional>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <vector>
#include <memory>
#include <list>
#include <tuple>
#include <uv.h>
#include "config.h"

UVW_MSVC_WARNING_PUSH_DISABLE_DLLINTERFACE();


namespace uvw {

    /**
     * @brief The ErrorEvent event.
     *
     * Custom wrapper around error constants of `libuv`.
     */
    struct UVW_EXTERN ErrorEvent {
        template<typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
        explicit ErrorEvent(U val) noexcept
            : ec{ static_cast<int>(val) }
        {}

        /**
         * @brief Returns the `libuv` error code equivalent to the given platform dependent error code.
         *
         * It returns:
         * * POSIX error codes on Unix (the ones stored in errno).
         * * Win32 error codes on Windows (those returned by GetLastError() or WSAGetLastError()).
         *
         * If `sys` is already a `libuv` error code, it is simply returned.
         *
         * @param sys A platform dependent error code.
         * @return The `libuv` error code equivalent to the given platform dependent error code.
         */
        static int translate(int sys) noexcept;

        /**
         * @brief Returns the error message for the given error code.
         *
         * Leaks a few bytes of memory when you call it with an unknown error code.
         *
         * @return The error message for the given error code.
         */
        const char* what() const noexcept;

        /**
         * @brief Returns the error name for the given error code.
         *
         * Leaks a few bytes of memory when you call it with an unknown error code.
         *
         * @return The error name for the given error code.
         */
        const char* name() const noexcept;

        /**
         * @brief Gets the underlying error code, that is an error constant of `libuv`.
         * @return The underlying error code.
         */
        int code() const noexcept;

        /**
         * @brief Checks if the event contains a valid error code.
         * @return True in case of success, false otherwise.
         */
        explicit operator bool() const noexcept;

    private:
        const int ec;
    };


    /**
     * @brief Event emitter base class.
     *
     * Almost everything in `uvw` is an event emitter.<br/>
     * This is the base class from which resources and loops inherit.
     */
    template<typename T, typename ...Events>
    class UVW_EXTERN Emitter {

        struct UVW_EXTERN BaseHandler {
            virtual ~BaseHandler() noexcept = default;
            virtual bool empty() const noexcept = 0;
            virtual void clear() noexcept = 0;
        };

        template<typename E>
        struct UVW_EXTERN Handler final: BaseHandler{
            using Listener = std::function<void(E&, T&)>;
            using Element = std::pair<bool, Listener>;
            using ListenerList = std::list<Element>;
            using Connection = typename ListenerList::iterator;

            bool empty() const noexcept override {
                auto pred = [](auto&& element) {
                    return element.first;
                };

                if (onceL.empty() && onL.empty()) {
                    return true;
                }
                return std::all_of(onceL.cbegin(), onceL.cend(), pred) &&
                        std::all_of(onL.cbegin(), onL.cend(), pred);
            }

            void clear() noexcept override {
                if (publishing) {
                    auto func = [](auto&& element) { element.first = true; };
                    std::for_each(onceL.begin(), onceL.end(), func);
                    std::for_each(onL.begin(), onL.end(), func);
                }
                else {
                    onceL.clear();
                    onL.clear();
                }
            }

            Connection once(Listener f) {
                return onceL.emplace(onceL.cend(), false, std::move(f));
            }

            Connection on(Listener f) {
                return onL.emplace(onL.cend(), false, std::move(f));
            }

            void erase(Connection conn) noexcept {
                conn->first = true;

                if (!publishing) {
                    auto pred = [](auto&& element) { return element.first; };
                    onceL.remove_if(pred);
                    onL.remove_if(pred);
                }
            }

            void publish(E event, T& ref) {
                ListenerList currentL;
                std::swap(onceL, currentL);
                
                auto func = [&event, &ref](auto&& element) {
                    return element.first ? void() : element.second(event, ref);
                };

                publishing = true;

                std::for_each(onL.rbegin(), onL.rend(), func);
                std::for_each(currentL.rbegin(), currentL.rend(), func);

                publishing = false;

                onL.remove_if([](auto&& element) { return element.first; });
            }

            private:
                bool publishing{false};
                ListenerList onceL{};
                ListenerList onL{};
        };

    protected:
        template<typename E>
        void publish(E event) {
            auto &handler = std::get<Handler<E>>(pools);
            handler.publish(std::move(event), *static_cast<T*>(this));
        }

    public:
        Emitter() = default;
        Emitter(Emitter&&) = default;
        Emitter& operator=(Emitter&&) = default;

        // These must be deleted because MSVC try to export them with UVW_EXPORT
        Emitter(const Emitter&) = delete;
        Emitter& operator=(const Emitter&) = delete;

        template<typename E>
        using Listener = typename Handler<E>::Listener;

        /**
         * @brief Connection type for a given event type.
         *
         * Given an event type `E`, `Connection<E>` is the type of the connection
         * object returned by the event emitter whenever a listener for the given
         * type is registered.
         */
        template<typename E>
        struct UVW_EXTERN Connection : private Handler<E>::Connection {
            template<typename, typename ...> friend class Emitter;

            Connection() = default;
            Connection(const Connection&) = default;
            Connection(Connection&&) = default;
            ~Connection() = default;

            Connection(typename Handler<E>::Connection conn)
                : Handler<E>::Connection{ std::move(conn) }
            {}

            Connection& operator=(const Connection&) = default;
            Connection& operator=(Connection&&) = default;
        };

        virtual ~Emitter() noexcept {
            static_assert(std::is_base_of_v<Emitter<T, Events...>, T>);
        }

        /**
         * @brief Registers a long-lived listener with the event emitter.
         *
         * This method can be used to register a listener that is meant to be
         * invoked more than once for the given event type.<br/>
         * The Connection object returned by the method can be freely discarded. It
         * can be used later to disconnect the listener, if needed.
         *
         * Listener is usually defined as a callable object assignable to a
         * `std::function<void(const E &, T &)`, where `E` is the type of the event
         * and `T` is the type of the resource.
         *
         * @param f A valid listener to be registered.
         * @return Connection object to be used later to disconnect the listener.
         */
        template<typename E>
        Connection<E> on(Listener<E> f) {
            auto &handler = std::get<Handler<E>>(pools);
            return handler.on(std::move(f));
        }

        /**
         * @brief Registers a short-lived listener with the event emitter.
         *
         * This method can be used to register a listener that is meant to be
         * invoked only once for the given event type.<br/>
         * The Connection object returned by the method can be freely discarded. It
         * can be used later to disconnect the listener, if needed.
         *
         * Listener is usually defined as a callable object assignable to a
         * `std::function<void(const E &, T &)`, where `E` is the type of the event
         * and `T` is the type of the resource.
         *
         * @param f A valid listener to be registered.
         * @return Connection object to be used later to disconnect the listener.
         */
        template<typename E>
        Connection<E> once(Listener<E> f) {
            return std::get<Handler<E>>(pools).once(std::move(f));
        }

        /**
         * @brief Disconnects a listener from the event emitter.
         * @param conn A valid Connection object
         */
        template<typename E>
        void erase(Connection<E> conn) noexcept {
            std::get<Handler<E>>(pools).erase(std::move(conn));
        }

        /**
         * @brief Disconnects all the listeners for the given event type.
         */
        template<typename E>
        void clear() noexcept {
            std::get<Handler<E>>(pools).clear();
        }

        /**
         * @brief Disconnects all the listeners.
         */
        void clear() noexcept {
            std::apply([](auto& ...x) {(..., x.clear()); }, pools);
        }

        /**
         * @brief Checks if there are listeners registered for the specific event.
         * @return True if there are no listeners registered for the specific event,
         * false otherwise.
         */
        template<typename E>
        bool empty() const noexcept {
            return std::get<Handler<E>>(pools).empty();
        }

        /**
         * @brief Checks if there are listeners registered with the event emitter.
         * @return True if there are no listeners registered with the event emitter,
         * false otherwise.
         */
        bool empty() const noexcept {
            auto empty{ true };
            std::apply([&empty](auto&&... args) {((empty &= args.empty()), ...); }, pools);
            return empty;
        }

    private:
        std::tuple<Handler<Events>...> pools{};
    };

    struct UVW_EXTERN FakeEvent { };

    struct UVW_EXTERN TestEmitter : Emitter<TestEmitter, FakeEvent, ErrorEvent> {
        void emit() { publish(FakeEvent{}); }
    };

    template struct UVW_EXTERN uvw::Emitter<TestEmitter, FakeEvent, ErrorEvent>::Connection<uvw::FakeEvent>;
    template void Emitter<TestEmitter, FakeEvent, ErrorEvent>::clear(void)noexcept;
    template bool uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::empty(void)const noexcept;
    template 
        uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
        uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
        ::on(std::function<void(struct uvw::ErrorEvent&, uvw::TestEmitter&)>);

    template struct UVW_EXTERN uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Connection<uvw::ErrorEvent>;

    
    template 
        uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>::Connection
        uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>
        ::once(std::function<void(struct uvw::FakeEvent&, uvw::TestEmitter&)>);

    template uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>::Connection
        uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>
        ::on(std::function<void(struct uvw::FakeEvent&, uvw::TestEmitter&)>);

    template void uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>::erase(uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Handler<uvw::FakeEvent>::Connection)noexcept;
    // template<> inline uvw::Emitter<uvw::TestEmitter, uvw::FakeEvent, uvw::ErrorEvent>::Emitter() = default;
}
#ifndef UVW_AS_LIB
#include "emitter.cpp"
#endif

UVW_MSVC_WARNING_POP();

#endif // UVW_EMITTER_INCLUDE_H
