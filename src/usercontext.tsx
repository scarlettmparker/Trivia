import { createContext, useContext, createSignal, JSX } from 'solid-js';

type UserContextType = {
  userId: () => number;
  setUserId: (id: number) => void;
  username: () => string;
  setUsername: (name: string) => void;
};

const UserContext = createContext<UserContextType>();

export const UserProvider = (props: { children: JSX.Element }) => {
  const [userId, setUserId] = createSignal<number>(-1);
  const [username, setUsername] = createSignal<string>("");

  return (
    <UserContext.Provider value={{ userId, setUserId, username, setUsername }}>
      {props.children}
    </UserContext.Provider>
  );
};

export const useUser = () => {
  const context = useContext(UserContext);
  if (!context) {
    throw new Error('useUser must be used within a UserProvider');
  }
  return context;
};