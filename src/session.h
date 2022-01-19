/**
*     Copyright (C) 2022 Mason Soroka-Gill
* 
*     This program is free software: you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation, either version 3 of the License, or
*     (at your option) any later version.
* 
*     This program is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
* 
*     You should have received a copy of the GNU General Public License
*     along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "libs.h"

#define SESSION_ID_LEN 32

namespace hussar {
    std::mutex openssl_rand_mtx;
    std::unique_lock<std::mutex> openssl_rand_lock(openssl_rand_mtx);

    struct Session {
        std::string id;
        std::unordered_map<std::string, std::string> data;

        Session()
        {
            unsigned char id_data[SESSION_ID_LEN];
            std::memset(id_data, 0, SESSION_ID_LEN);
            std::ostringstream oss;

            openssl_rand_lock.lock();
                int error = RAND_bytes(id_data, SESSION_ID_LEN);
            openssl_rand_lock.unlock();

            if (error == -1) {
                print_lock.lock();
                std::cerr << "Not supported by the current RAND method.";
                print_lock.unlock();
                std::exit(1);
            } else if (error == 0) {
                print_lock.lock();
                std::cerr << "Other openssl error.";
                print_lock.unlock();
                std::exit(1);
            }

            for (size_t n = 0; n < SESSION_ID_LEN; ++n) {
                oss << std::hex << (int)id_data[n];
            }

            id = oss.str();
        }

        Session(Session&& old_session)
        {
            this->id = std::move(old_session.id);
            this->data = std::move(old_session.data);
        }

        Session& operator=(Session&& old_session)
        {
            this->id = std::move(old_session.id);
            this->data = std::move(old_session.data);
            return *this;
        }

        // delete copy constructors
        Session(Session& old_session) = delete;
        Session(const Session& old_session) = delete;
        Session& operator=(Session& old_session) = delete;
        Session& operator=(const Session& old_session) = delete;
    };

    std::unordered_map<std::string, Session> sessions;
    std::mutex sessions_mtx;
    std::unique_lock<std::mutex> sessions_lock(sessions_mtx);

    /**
     * Creates a session and returns its id
     */
    std::string create_session()
    {
        sessions_lock.lock();
            Session s;
            std::string session_id = s.id;
            sessions[s.id] = std::move(s);
        sessions_lock.unlock();
        return session_id;
    }

    /**
     * reads value of session key if it exists and returns it, else returns empty string
     */
    std::string read_session(const std::string& session_id, const std::string& key)
    {
        std::string data;
        sessions_lock.lock();
            auto session_iter = sessions.find(session_id);
            if (session_iter != sessions.end()) {
                auto session_data_iter = session_iter->second.data.find(key);
                if (session_data_iter != session_iter->second.data.end()) {
                    data = session_data_iter->second;
                }
            }
        sessions_lock.unlock();
        return data;
    }

    /**
     * writes data to session if it exists then return true if the session data was overwritten or created
     */
    bool write_session(const std::string& session_id, const std::string& key, const std::string& data)
    {
        bool success = false;
        sessions_lock.lock();
            auto session_iter = sessions.find(session_id);
            if (session_iter != sessions.end()) {
                session_iter->second.data[key] = data;
                success = true;
            }
        sessions_lock.unlock();
        return success;
    }

    /**
     * deletes session data
     */
    bool delete_session(const std::string& session_id, const std::string& key)
    {
        bool success = false;
        sessions_lock.lock();
            auto session_iter = sessions.find(session_id);
            if (session_iter != sessions.end()) {
                auto session_data_iter = session_iter->second.data.find(key);
                if (session_data_iter != session_iter->second.data.end()) {
                    session_iter->second.data.erase(session_data_iter);
                    success = true;
                }
            }
        sessions_lock.unlock();
        return success;
    }

    /**
     * deletes session from session pool
     */
    bool destroy_session(const std::string& session_id)
    {
        bool success = false;
        sessions_lock.lock();
            auto session_iter = sessions.find(session_id);
            if (session_iter != sessions.end()) {
                sessions.erase(session_iter);
                success = true;
            }
        sessions_lock.unlock();
        return success;
    }

    /**
     * returns whether or not the session exists
     */
    bool session_exists(const std::string& session_id)
    {
        bool success = false;
        sessions_lock.lock();
            success = sessions.find(session_id) != sessions.end();
        sessions_lock.unlock();
        return success;
    }
};

