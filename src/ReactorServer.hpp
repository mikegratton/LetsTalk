#pragma once
#include <memory>

#include "Reactor.hpp"
namespace lt {

/**
 * The ReactorServer handles the server-side of the reactor pattern. This is a pull-style API.
 * The ReactorServer provides methods to find about pending sessions and interact with them, but
 * does not run callbacks that do the work. That's done by your code.
 *
 * By default, the ProgressData type is set to reactor_void_progress -- that is, there's no
 * associated progress data, just integer progress marks. This enables using the simplified
 * progress() call below.
 */
template <class Req, class Rep, class ProgressData = reactor_void_progress>
class ReactorServer : public std::enable_shared_from_this<ReactorServer<Req, Rep, ProgressData>> {
    using Backend = detail::ReactorServerBackend<Req, Rep, ProgressData>;

   public:
    /// Each request starts a session, modeled by an instance of this class
    class Session {
       public:
        /// Inspect the request data for this session
        Req const* request() const { return m_request.get(); }

        /**
         * Transmit progress information to the client
         * @param i_progress Current progress mark
         * @param i_data Related progress data
         */
        void progress(int i_progress, ProgressData const& i_data);

        /**
         * Transmit progress information to the client. This is a simplified version for when there's no
         * progress data, just marks.
         * @param i_progress Current progress mark
         */
        typename std::enable_if<std::is_same<ProgressData, reactor_void_progress>::value>::type progress(int i_progress)
        {
            progress(i_progress, reactor_void_progress());
        }

        /// Check that the client hasn't cancelled this session, or that it isn't already complete
        bool isAlive() const;

        /// Send the reply, ending the session
        void reply(Rep const& i_reply);

        /// Mark the request as failed
        void fail();

       protected:
        friend Backend;

        Session(std::shared_ptr<Backend> i_reactor, Req const& i_request, Guid i_id);

        std::shared_ptr<Backend> m_reactor;  /// All calls are forwarded to the reactor server object
        Req m_request;                       /// Request that kicked off this session
        Guid m_id;                           /// Id of this session
    };

    /// Check if there is a pending session
    bool havePendingSession() const;

    /// Get the next pending request as a new session object. If there are no pending requests,
    /// this retuns a session object where isAlive() returns false
    Session getPendingSession(std::chrono::nanoseconds i_wait = std::chrono::nanoseconds(0));

    /// For debugging. Prints connection status if LT_VERBOSE is defined
    void logConnectionStatus() const;

   protected:
    friend class Participant;
    ReactorServer(std::shared_ptr<Backend> i_backend);
    std::shared_ptr<Backend> m_backend;
};

}  // namespace lt