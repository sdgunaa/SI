import { QueryClient } from "@tanstack/react-query";

// SI uses Electron IPC exclusively - no HTTP API calls needed
// QueryClient is used only for local state management and caching
export const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      refetchInterval: false,
      refetchOnWindowFocus: false,
      staleTime: Infinity,
      retry: false,
    },
    mutations: {
      retry: false,
    },
  },
});
