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

#include "hussar.h"

void redirect_home(hus::Request& req, hus::Response& resp) {
    resp.code = "302";
    resp.headers["Location"] = "/";
}

void home(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        resp.body = "<h1>Welcome to the website, <b>" + hus::html_escape(hus::read_session(req.session_id, "username")) + "</b>!</h1><br><p>Click <a href=\"/logout\">HERE</a> to log out.</p>";
    } else {
        resp.body = "<h1>Welcome to the website!</h1><br>"
                    "<p>Click <a href=\"/login\">HERE</a> to go to the login page</p>"
                    "<p>Click <a href=\"/upload\">HERE</a> to go to the upload page</p>";
    }
}

void login_page(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        redirect_home(req, resp);
        return;
    }

    // multiline string
    resp.body = R"(
<form action="/login" method="post">
<label for="name">Username: </label>
<input type="text" id="username" name="username" required><br>
<label for="password">Password: </label>
<input type="password" id="password" name="password" required><br>
<input type="submit" value="Submit">
    )";

    // checking if parameter exists
    if (req.get.find("message") != req.get.end()) {
        resp.body += "<br><p>" + hus::html_escape(req.get["message"]) + "</p>";
    }
}

void login(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        redirect_home(req, resp);
        return;
    }

    // Checking if post parameters exist
    if (req.post.find("username") == req.post.end()) goto login_redirect;
    if (req.post.find("password") == req.post.end()) goto login_redirect;

    // TODO get creds from a db query and do a hash of some kind
    if (req.post["password"] != "lemon42") goto login_redirect; // TODO compute hash from given password and compare with hash from db
    if (hus::write_session(req.session_id, "username", req.post["username"])) {
        redirect_home(req, resp);
    } else {
        goto login_redirect;
    }

    return;

    // If anything here fails, return to the login page with a generic error
login_redirect:
    resp.code = "302";
    resp.headers["Location"] = "/login?message=Failed+to+login.";
}

void logout(hus::Request& req, hus::Response& resp) {
    resp.code = "302";
    if (hus::delete_session(req.session_id, "username")) {
        resp.headers["Location"] = "/login?message=Logged+out.";
        hus::destroy_session(req.session_id);
    } else {
        resp.headers["Location"] = "/login?message=Not+logged+in.";
    }
}

void upload_page(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        redirect_home(req, resp);
        return;
    }

    // multiline string
    resp.body = R"(
<script>
function upload() {
    let files = document.getElementById("file").files;
    if (files.length === 0) {
        return;
    }
    let ajax = new XMLHttpRequest;

    let formData = new FormData;
    formData.append('file', files[0]);

    ajax.open("PUT", "/upload", true);
    ajax.send(formData);
}
</script>
<form action="/upload" method="POST">
<label for="file">File: </label>
<input type="file" id="file" name="file" required><br>
<input type="button" value="Submit" onclick=upload()>
</form>
    )";

    // checking if parameter exists
    if (req.get.find("message") != req.get.end()) {
        resp.body += "<br><p>" + hus::html_escape(req.get["message"]) + "</p>";
    }
}

void upload(hus::Request& req, hus::Response& resp) {
    resp.body = "<h2>HELLO WORLD</h2>";
    return;
}

int main() {
    hus::Config config;
    config.host         = "127.0.0.1"; // Socket Host
    config.port         = 8443;        // Socket Port
    config.thread_count = 0;           // Thread count
    config.private_key  = "key.pem";   // SSL Private key
    config.certificate  = "cert.pem";  // SSL Public key
    config.verbosity    = 1;           // Verbosity enabled

    // set config.private_key and config.certificate to "" for no ssl
    hus::Hussar s(config);

    // register routes
    s.fallback(&redirect_home);       // fallback route is everything other than registered routes
    s.get("/", &home);
    s.get("/login", &login_page);
    s.post("/login", &login);
    s.get("/logout", &logout);
    s.get("/upload", &upload_page);
    s.alt("PUT", "/upload", &upload);

    // BLOCKING listen for the server
    // perhaps put in a thread for non-blocking style behaviour so
    // multiple hus::Hussar instances can exist in the same process
    s.serve();

    return 0;
}

