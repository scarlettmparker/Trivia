import { createSignal } from "solid-js";
import ButtonWithAltProps from "./buttonwithaltprops";
import styles from './buttonwithalt.module.css';

/**
 * Button with an image and an alt text that appears when the mouse hovers over the button.
 * @param src Source of the image.
 * @param alt_text Alt text of the image.
 * @param class Class of the container div.
 * @param button_class Class of the button.
 * @param width Width of the button.
 * @param height Height of the button.
 * @param draggable Whether the image is draggable.
 * @param onclick Function to call when the button is clicked.
 * @returns JSX Element - button with an image and alt text.
 */
const ButtonWithAlt = ({ src, alt_text, class: class_name, button_class, width, height, draggable, onclick: onclick_ }: ButtonWithAltProps) => {
  const [mouseover, setMouseover] = createSignal(false);

  return (
    <div class={class_name} onmouseover={() => setMouseover(true)} onmouseleave={() => setMouseover(false)} >
      {mouseover() && <span class={styles.alt_text}>{alt_text()}</span>}
      <button class={button_class} onclick={onclick_} aria-label={alt_text()} style={{ width: `${width}px`, height: `${height}px` }} tabindex={-1}>
        <img src={src()} class={styles.alt_image} width={width} height={height} draggable={draggable} alt={alt_text()} />
      </button>
    </div>
  );
}

export default ButtonWithAlt;