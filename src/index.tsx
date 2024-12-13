import { render } from "solid-js/web";
import { Router, Route } from "@solidjs/router";
import { MetaProvider } from "@solidjs/meta";
import "./index.css";

import Index from './routes/index';
import Login from './routes/login';

render(
  () => (
    <MetaProvider>
      <Router>
        <Route path="/" component={Index} />
        <Route path="/login" component={Login} />
      </Router>
    </MetaProvider>
  ),
  document.getElementById("root")!
);
