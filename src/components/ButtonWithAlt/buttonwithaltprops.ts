interface ButtonWithAltProps {
  src: () => string;
  alt_text: () => string;
  class?: string;
  button_class?: string;
  width?: number;
  height?: number;
  draggable?: boolean;
  onclick?: () => void;
}

export default ButtonWithAltProps;