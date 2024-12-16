import { render } from "solid-js/web";
import { Router, Route } from "@solidjs/router";
import { MetaProvider } from "@solidjs/meta";
import "./index.css";

import Index from './routes/index';
import Login from './routes/login';
import Admin from "./routes/admin";
import { UserProvider } from "./usercontext";

render(
  () => (
    <UserProvider>
      <MetaProvider>
        <Router>
          <Route path="/" component={Index} />
          <Route path="/login" component={Login} />
          <Route path="/admin" component={Admin} />
        </Router>
      </MetaProvider>
    </UserProvider>
  ),
  document.getElementById("root")!
);
