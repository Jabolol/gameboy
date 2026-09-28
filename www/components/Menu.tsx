import type { ComponentChildren, JSX, SVGAttributes } from "preact";

type Icon = (props: SVGAttributes<SVGSVGElement>) => JSX.Element;

interface RowLabelProps {
  icon: Icon;
  label: string;
  detail?: string;
}

interface MenuActionProps extends RowLabelProps {
  trailing?: ComponentChildren;
  onClick: () => void;
}

interface MenuLinkProps extends RowLabelProps {
  href: string;
  trailing?: ComponentChildren;
}

interface MenuSwitchProps extends RowLabelProps {
  checked: boolean;
  onChange: (checked: boolean) => void;
}

export interface Choice<T> {
  value: T;
  label: string;
  icon?: Icon;
}

export interface Swatch<T> {
  value: T;
  label: string;
  fill: string;
}

interface MenuSwatchesProps<T> extends RowLabelProps {
  swatches: readonly Swatch<T>[];
  value: T;
  onChange: (value: T) => void;
}

interface MenuChoiceProps<T> extends RowLabelProps {
  choices: readonly Choice<T>[];
  value: T;
  onChange: (value: T) => void;
}

const ROW_CLASSES =
  "flex items-center gap-3 w-full min-h-11 px-3 py-1.5 rounded-2xl text-left text-[15px] select-none";

const PRESSABLE_CLASSES =
  "hover:bg-[var(--fill)] active:bg-[var(--fill-strong)]";

const SEGMENT_CLASSES =
  "grid place-items-center min-w-8 h-7 px-2 rounded-full text-[13px] font-medium text-[var(--label-secondary)] aria-checked:text-[var(--label)] aria-checked:bg-white dark:aria-checked:bg-white/20 aria-checked:shadow-[0_1px_4px_rgb(0_0_0/0.14)]";

function RowLabel({ icon: Icon, label, detail }: RowLabelProps) {
  return (
    <>
      <Icon class="w-5 h-5 shrink-0" />
      <span class="flex flex-col flex-1 min-w-0 leading-tight">
        <span>{label}</span>
        {detail && (
          <span class="text-[13px] text-[var(--label-secondary)]">
            {detail}
          </span>
        )}
      </span>
    </>
  );
}

export function MenuAction(
  { trailing, onClick, ...label }: MenuActionProps,
) {
  return (
    <button
      type="button"
      class={`${ROW_CLASSES} ${PRESSABLE_CLASSES}`}
      onClick={onClick}
    >
      <RowLabel {...label} />
      <span class="text-[var(--label-secondary)]">{trailing}</span>
    </button>
  );
}

export function MenuLink({ href, trailing, ...label }: MenuLinkProps) {
  return (
    <a
      href={href}
      target="_blank"
      rel="noopener noreferrer"
      class={`${ROW_CLASSES} ${PRESSABLE_CLASSES}`}
    >
      <RowLabel {...label} />
      <span class="text-[var(--label-secondary)]">{trailing}</span>
    </a>
  );
}

export function MenuSwitch(
  { checked, onChange, ...label }: MenuSwitchProps,
) {
  return (
    <button
      type="button"
      role="switch"
      aria-checked={checked}
      class={`${ROW_CLASSES} hover:bg-[var(--fill)] group`}
      onClick={() => onChange(!checked)}
    >
      <RowLabel {...label} />
      <span class="relative w-[38px] h-[22px] shrink-0 rounded-full bg-[var(--fill-strong)] group-aria-checked:bg-[#34c759]">
        <span class="absolute top-[2px] left-[2px] w-[18px] h-[18px] rounded-full bg-white shadow-[0_1px_3px_rgb(0_0_0/0.3)] transition-transform duration-200 ease-out group-aria-checked:translate-x-4" />
      </span>
    </button>
  );
}

export function MenuChoice<T extends string | number>(
  { choices, value, onChange, ...label }: MenuChoiceProps<T>,
) {
  return (
    <div class={ROW_CLASSES}>
      <RowLabel {...label} />
      <div
        role="radiogroup"
        aria-label={label.label}
        class="flex shrink-0 p-0.5 rounded-full bg-[var(--fill)]"
      >
        {choices.map((choice) => (
          <button
            key={choice.value}
            type="button"
            role="radio"
            aria-checked={choice.value === value}
            aria-label={choice.icon && choice.label}
            title={choice.icon && choice.label}
            class={SEGMENT_CLASSES}
            onClick={() => onChange(choice.value)}
          >
            {choice.icon ? <choice.icon class="w-4 h-4" /> : choice.label}
          </button>
        ))}
      </div>
    </div>
  );
}

export function MenuSwatches<T extends string>(
  { swatches, value, onChange, ...label }: MenuSwatchesProps<T>,
) {
  return (
    <div class={`${ROW_CLASSES} flex-col items-stretch`}>
      <div class="flex items-center gap-3">
        <RowLabel {...label} />
      </div>
      <div
        role="radiogroup"
        aria-label={label.label}
        class="flex flex-wrap gap-2 pl-8 pb-1"
      >
        {swatches.map((swatch) => (
          <button
            key={swatch.value}
            type="button"
            role="radio"
            aria-checked={swatch.value === value}
            aria-label={swatch.label}
            title={swatch.label}
            class="w-6 h-6 rounded-full shadow-[inset_0_0_0_0.5px_rgb(0_0_0/0.2),inset_0_1px_1px_rgb(255_255_255/0.4)] outline-2 outline-offset-2 outline-transparent aria-checked:outline-[var(--label)]"
            style={{ background: swatch.fill }}
            onClick={() => onChange(swatch.value)}
          />
        ))}
      </div>
    </div>
  );
}

export function MenuSeparator() {
  return <div role="separator" class="h-px mx-3 my-1 bg-[var(--separator)]" />;
}
