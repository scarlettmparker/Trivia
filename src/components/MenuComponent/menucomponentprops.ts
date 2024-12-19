interface MenuComponentProps {
  name: string;
  class?: () => string;
  onclick?: (e: MouseEvent) => void;
  onmouseover?: (e: MouseEvent) => void;
}

export default MenuComponentProps;