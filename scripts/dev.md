# Dev loop (fast iteration)

Terminal 1 — React with hot reload:
    cd frontend && npm install && npm run dev
    # serves at http://localhost:5173

Terminal 2 — point the C++ app at the dev server instead of ui/index.html:
    edit src/main.cpp -> w.navigate("http://localhost:5173");
    cmake --preset default && cmake --build build && ./build/app

For release, revert navigate() to "file://ui/index.html" and run npm run build.
