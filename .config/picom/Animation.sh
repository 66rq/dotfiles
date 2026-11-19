#!/bin/sh

SCRIPT_DIR=$(CDPATH= cd "$(dirname "$0")" 2>/dev/null && pwd)
if [ -z "$SCRIPT_DIR" ]; then
    printf '%s\n' "Cannot locate the project directory."
    exit 1
fi

CONFIG="$SCRIPT_DIR/picom.conf"
PROFILES_DIR="$SCRIPT_DIR/modules/profiles"
ADDONS_DIR="$SCRIPT_DIR/modules/Addons"

if [ ! -d "$PROFILES_DIR" ] || [ ! -d "$ADDONS_DIR" ]; then
    printf '%s\n' "Missing modules directory. Keep Animation.sh beside picom.conf and modules/."
    exit 1
fi

if [ ! -t 0 ] || [ ! -r /dev/tty ] || [ ! -w /dev/tty ]; then
    printf '%s\n' 'Animation.sh needs to run in an interactive terminal.'
    exit 1
fi

for utility in stty dd od tr grep cp mv; do
    if ! command -v "$utility" >/dev/null 2>&1; then
        printf 'Required command not found: %s\n' "$utility"
        exit 1
    fi
done

PROFILE=pop
WORKSPACE=on
WINDOW=on
OPACITY=on
CURSOR=0
MESSAGE=''
DONE=0
EXIT_CODE=0

has_include() {
    grep -Eq "^[[:space:]]*@include[[:space:]]+\"$1\"[[:space:]]*$" "$CONFIG" 2>/dev/null
}

load_settings() {
    if [ -f "$CONFIG" ]; then
        PROFILE=none
        for candidate in fade slide pop fly appear; do
            if has_include "modules/profiles/$candidate.conf"; then
                PROFILE=$candidate
                break
            fi
        done
        if has_include 'modules/profiles/custom.conf'; then
            PROFILE=custom
        fi

        if has_include 'modules/Addons/workspace.conf'; then WORKSPACE=on; else WORKSPACE=off; fi
        if has_include 'modules/Addons/window.conf'; then WINDOW=on; else WINDOW=off; fi
        if has_include 'modules/Addons/opacity.conf'; then OPACITY=on; else OPACITY=off; fi
    fi

    if [ "$PROFILE" = custom ]; then
        WINDOW=off
    fi
}

profile_label() {
    case "$1" in
        fade) printf '%s' 'Fade' ;;
        slide) printf '%s' 'Slide' ;;
        pop) printf '%s' 'Pop' ;;
        fly) printf '%s' 'Fly' ;;
        appear) printf '%s' 'Appear / Disappear' ;;
        custom) printf '%s' 'Custom (reference animation)' ;;
        *) printf '%s' 'Disabled' ;;
    esac
}

status_mark() {
    if [ "$1" = on ]; then printf '%s' '[x]'; else printf '%s' '[ ]'; fi
}

render_profile_row() {
    row_id=$1
    name=$2
    label=$3
    if [ "$PROFILE" = "$name" ]; then mark='(*)'; else mark='( )'; fi
    draw_row "$row_id" "$mark  $label"
}

render_addon_row() {
    row_id=$1
    name=$2
    label=$3
    if [ "$name" = workspace ]; then value=$WORKSPACE
    elif [ "$name" = window ]; then value=$WINDOW
    else value=$OPACITY
    fi
    mark=$(status_mark "$value")
    draw_row "$row_id" "$mark  $label"
}

draw_row() {
    row_id=$1
    row_text=$2
    if [ "$CURSOR" -eq "$row_id" ]; then
        printf '\033[7m  > %s  \033[0m\n' "$row_text"
    else
        printf '    %s\n' "$row_text"
    fi
}

render_screen() {
    printf '\033[2J\033[H'
    printf '\033[1;36mPICOM CONFIGURATOR\033[0m\n'
    printf '%s\n' 'Use ↑/↓ or j/k to move. Space/Enter to select or toggle. q to quit.'
    printf '%s\n' '--------------------------------------------------------------------'
    printf '\033[1mAnimation profile (choose one)\033[0m\n'
    render_profile_row 0 none 'Disabled'
    render_profile_row 1 fade 'Fade'
    render_profile_row 2 slide 'Slide'
    render_profile_row 3 pop 'Pop'
    render_profile_row 4 fly 'Fly'
    render_profile_row 5 appear 'Appear / Disappear'
    render_profile_row 6 custom 'Custom (reference animation)'
    printf '\n\033[1mAddons\033[0m\n'
    render_addon_row 7 workspace 'Workspace transition'
    if [ "$PROFILE" = custom ]; then
        draw_row 8 '[locked] Window movement / resize (included in Custom)'
    else
        render_addon_row 8 window 'Window movement / resize'
    fi
    render_addon_row 9 opacity 'Application opacity rules'
    printf '\n\033[1mActions\033[0m\n'
    draw_row 10 'Save configuration'
    draw_row 11 'Save and restart Picom'
    draw_row 12 'Quit without saving'
    printf '\n'
    if [ -n "$MESSAGE" ]; then
        printf '\033[1;33m%s\033[0m\n' "$MESSAGE"
    else
        printf '%s\n' 'Space toggles the selected item. Profiles are mutually exclusive.'
    fi
}

render_profile_include() {
    name=$1
    if [ "$PROFILE" = "$name" ]; then
        printf '    @include "modules/profiles/%s.conf"\n' "$name"
    else
        printf '    # @include "modules/profiles/%s.conf"\n' "$name"
    fi
}

render_addon_include() {
    enabled=$1
    path=$2
    if [ "$enabled" = on ]; then
        printf '    @include "%s"\n' "$path"
    else
        printf '    # @include "%s"\n' "$path"
    fi
}

save_config() {
    tmp="$CONFIG.tmp.$$"
    umask 077

    {
        printf '%s\n' 'backend = "xrender";' 'vsync = true;' 'use-damage = true;' '' 'animations = ('
        printf '%s\n' '    # Choose ONE profile only.'
        render_profile_include fade
        render_profile_include slide
        render_profile_include pop
        render_profile_include fly
        render_profile_include appear
        if [ "$PROFILE" = custom ]; then
            printf '%s\n' '    # Custom is a Window Rule profile and is included below.'
        else
            printf '%s\n' '    # @include "modules/profiles/custom.conf"'
        fi
        render_addon_include "$WORKSPACE" 'modules/Addons/workspace.conf'
        printf '%s\n' ');' '' 'rules = ('
        render_addon_include "$OPACITY" 'modules/Addons/opacity.conf'

        if [ "$PROFILE" = custom ]; then
            printf '%s\n' '    @include "modules/profiles/custom.conf"'
            printf '%s\n' '    # Window addon is disabled because Custom includes geometry animation.'
            printf '%s\n' '    # @include "modules/Addons/window.conf"'
        else
            printf '%s\n' '    # @include "modules/profiles/custom.conf"'
            render_addon_include "$WINDOW" 'modules/Addons/window.conf'
        fi
        printf '%s\n' ');'
    } > "$tmp" || {
        rm -f "$tmp"
        MESSAGE='Could not write picom.conf.'
        return 1
    }

    if [ -f "$CONFIG" ]; then
        cp "$CONFIG" "$CONFIG.bak" || {
            rm -f "$tmp"
            MESSAGE='Could not create picom.conf.bak.'
            return 1
        }
    fi

    mv "$tmp" "$CONFIG" || {
        rm -f "$tmp"
        MESSAGE='Could not replace picom.conf.'
        return 1
    }

    MESSAGE='Configuration saved. Backup: picom.conf.bak'
    return 0
}

restart_picom() {
    if ! command -v picom >/dev/null 2>&1; then
        MESSAGE='Saved, but picom was not found in PATH.'
        return 1
    fi

    if ! command -v pkill >/dev/null 2>&1; then
        MESSAGE='Saved. pkill is unavailable; restart Picom manually.'
        return 1
    fi

    pkill -x picom 2>/dev/null || :
    sleep 1
    if picom --config "$CONFIG" --daemon >/dev/null 2>&1; then
        MESSAGE='Configuration saved and Picom restarted.'
        return 0
    fi

    MESSAGE='Configuration saved, but Picom failed to restart. Check picom.conf.'
    return 1
}

activate_row() {
    case "$CURSOR" in
        0) PROFILE=none ;;
        1) PROFILE=fade ;;
        2) PROFILE=slide ;;
        3) PROFILE=pop ;;
        4) PROFILE=fly ;;
        5) PROFILE=appear ;;
        6)
            PROFILE=custom
            WINDOW=off
            MESSAGE='Custom includes its own geometry animation; Window addon was turned off.'
            ;;
        7)
            if [ "$WORKSPACE" = on ]; then WORKSPACE=off; else WORKSPACE=on; fi
            ;;
        8)
            if [ "$PROFILE" = custom ]; then
                MESSAGE='Window addon cannot be enabled with Custom; Custom already handles geometry.'
            elif [ "$WINDOW" = on ]; then WINDOW=off
            else WINDOW=on
            fi
            ;;
        9)
            if [ "$OPACITY" = on ]; then OPACITY=off; else OPACITY=on; fi
            ;;
        10) save_config ;;
        11)
            if save_config; then restart_picom; fi
            ;;
        12) DONE=1 ;;
    esac
}

move_cursor() {
    direction=$1
    if [ "$direction" = down ]; then
        CURSOR=$((CURSOR + 1))
        [ "$CURSOR" -le 12 ] || CURSOR=0
    else
        CURSOR=$((CURSOR - 1))
        [ "$CURSOR" -ge 0 ] || CURSOR=12
    fi
}

read_byte() {
    BYTE=$(dd if=/dev/tty bs=1 count=1 2>/dev/null | od -An -tu1 | tr -d ' \n')
}

read_key() {
    KEY=other
    read_byte
    case "$BYTE" in
        65) KEY=up ;;       # A (after ESC [)
        66) KEY=down ;;     # B (after ESC [)
        10|13) KEY=enter ;;
        32) KEY=space ;;
        106) KEY=down ;;   # j
        107) KEY=up ;;     # k
        113|81) KEY=quit ;; # q / Q
        27)
            read_byte
            case "$BYTE" in
                91|79)
                    read_byte
                    case "$BYTE" in
                        65) KEY=up ;;
                        66) KEY=down ;;
                        67) KEY=right ;;
                        68) KEY=left ;;
                    esac
                    ;;
            esac
            ;;
    esac
}

OLD_STTY=$(stty -g < /dev/tty 2>/dev/null) || {
    printf '%s\n' 'Could not read terminal settings.'
    exit 1
}

cleanup() {
    stty "$OLD_STTY" < /dev/tty 2>/dev/null || :
    printf '\033[0m\033[?25h\n'
}
trap cleanup 0
trap 'exit 130' INT
trap 'exit 129' HUP
trap 'exit 143' TERM

load_settings

stty -echo -icanon min 0 time 1 < /dev/tty || {
    printf '%s\n' 'Could not enable interactive terminal mode.'
    exit 1
}
printf '\033[?25l'

while [ "$DONE" -eq 0 ]; do
    render_screen
    read_key
    MESSAGE=''
    case "$KEY" in
        up) move_cursor up ;;
        down) move_cursor down ;;
        space|enter) activate_row ;;
        quit) DONE=1 ;;
    esac
done

printf '\033[2J\033[H'
printf '%s\n' 'Picom configurator closed.'
exit "$EXIT_CODE"
