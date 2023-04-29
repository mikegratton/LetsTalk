#pragma once
#include <condition_variable>
#include <mutex>

#include "Reactor.hpp"

namespace lt {

/**
 * The ReactorClient controls the client side of a reactor. It can start multiple sessions, possibly
 * active simultaneously.
 *
 * The workflow is
 * 1. Send a request, starting a new session.
 * 2. Use the session object to check on progress.
 * 3. Get the reply, waiting as necessary.
 *
 */
template <class Req, class Rep, class ProgressData = reactor_void_progress>
class ReactorClient {
    using Backend = detail::ReactorClientBackend<Req, Rep, ProgressData>;

   public:
    /**
     * Each session is modeled by this class. It is lightweight and copyable.
     */
    class Session {
       public:
        /// Get the current progress (0 to 100)
        int progress() const;

        /**
         * Get the next progress data sample.
         * @param o_data The returned progress data is written here.
         * @param i_wait Time to wait for data to appear. By default, return immediately if there is no pending data
         * @return true if o_data contains a new sample.
         */
        bool progressData(ProgressData& o_data, std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

        /// Check if this session is alive
        bool isAlive() const;

        /// Cancel the request, ending the session
        void cancel();

        /**
         *  Get the response. Will return false if the response is not ready by i_wait
         * @param o_reply The returned reply data is written here.
         * @param i_wait Time to wait for data to appear. By default, return immediately if there is no pending data
         * @return true of o_data has the reply
         */
        bool get(Rep& o_reply, std::chrono::nanoseconds const& i_wait = std::chrono::nanoseconds(0));

       protected:
        friend Backend;
        Session(std::shared_ptr<Backend> m_reactor, Guid i_id);

        std::shared_ptr<Backend> m_reactor;  /// All functions just forward to this
        Guid m_id;                           /// Id of this session
    };

    /**
     * Make a request to the reactor service, starting a new session
     * @param i_request Request data
     * @return Session object used to interact with this session
     */
    Session request(Req const& i_request);

    /// Logs debug information about the connections if LT_VERBOSE is defined
    void logConnectionStatus() const;

   protected:
    friend class Participant;
    ReactorClient(std::shared_ptr<Backend> i_backend);

    std::shared_ptr<Backend> m_backend;
};

}  // namespace lt
