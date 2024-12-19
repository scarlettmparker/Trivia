import { Accessor } from "solid-js";

interface ListComponentProps {
  name: string;
  id: number;
  class?: string;
  currentmenu: Accessor<number>;
  onclick?: (e: MouseEvent) => void;
}

export default ListComponentProps;