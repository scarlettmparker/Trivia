import { Accessor } from "solid-js";

interface FilterMenuProps {
  filter: Accessor<number>;
  setFilter: (filter: number) => void;
}

export default FilterMenuProps;