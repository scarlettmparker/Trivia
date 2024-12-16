import { createContext, useContext, createSignal, JSX } from 'solid-js';

type UserContextType = {
  userId: () => number;
  setUserId: (id: number) => void;
  username: () => string;
  setUsername: (name: string) => void;
  superuser: () => boolean;
  setSuperuser: (superuser: boolean) => void;
};

const UserContext = createContext<UserContextType>();

export const UserProvider = (props: { children: JSX.Element }) => {
  const [userId, setUserId] = createSignal<number>(-1);
  const [username, setUsername] = createSignal<string>("");
  const [superuser, setSuperuser] = createSignal<boolean>(false);

  return (
    <UserContext.Provider value={{ userId, setUserId, username, setUsername, superuser, setSuperuser }}>
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