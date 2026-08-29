// binutils - symbol.rs

use libc::c_void;
use std::ffi::CStr;
use std::marker::PhantomData;

use bfd::BfdRaw;
use helpers::{
    get_symbol_flags, get_symbol_name, get_symbol_section_name,
    get_symbol_type, get_symbol_value, is_symbol_undefined,
	macro_bfd_make_empty_symbol, macro_bfd_minisymbol_to_symbol,
};

pub enum SymbolRaw {}

/* No destructor because memory allocated by bfd_make_empty_symbol
is managed by the bfd object */
#[derive(Clone, Copy)]
pub struct Symbol<'a> {
    bfd: *mut BfdRaw,
    bfds: *mut SymbolRaw,
    _phantom: PhantomData<&'a ()>,
}

impl<'a> Symbol<'a> {
    pub(crate) fn from_minisymbol(bfd: *mut BfdRaw, ms: *const c_void) -> Symbol<'a> {
		let mut sr = unsafe {
			macro_bfd_make_empty_symbol(bfd as *mut BfdRaw)
		};
		unsafe { 
			if sr.is_null() {
				panic!()
			};
			/*
			bfd_minisymbol_to_symbol might use the empty symbol or it might not. 
			*/
			sr = macro_bfd_minisymbol_to_symbol(
				bfd as *mut BfdRaw,
				false,
				ms,
				sr
			)
		};
		Symbol { bfd, bfds: sr, _phantom: PhantomData }
    }

    pub fn name(&self) -> String {
        unsafe {
            let ptr = get_symbol_name(self.bfds);
            if ptr.is_null() {
                "(null)".to_string()
            } else {
                CStr::from_ptr(ptr).to_string_lossy().into_owned()
            }
        }
    }

    pub fn value(&self) -> u64 {
        unsafe { get_symbol_value(self.bfds) as u64 }
    }

    pub fn type_char(&self) -> char {
        unsafe {
            let c = get_symbol_type(self.bfd, self.bfds);
            if c == 0 { '-' } else { c as u8 as char }
        }
    }

    pub fn flags(&self) -> u64 {
        unsafe { get_symbol_flags(self.bfds) as u64 }
    }

    pub fn sec_name(&self) -> String {
        unsafe {
            let ptr = get_symbol_section_name(self.bfds);
            if ptr.is_null() {
                String::from("*ABS*")
            } else {
                CStr::from_ptr(ptr).to_string_lossy().into_owned()
            }
        }
    }

    pub fn is_undefined(&self) -> bool {
		unsafe { is_symbol_undefined(self.bfds) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use bfd::{Bfd, BfdFormat};

    fn open_test_object() -> Bfd {
        let bfd = Bfd::openr("tests/thunar-notify.c.o", "elf64-x86-64").unwrap();
        let mut bfd = bfd;
        bfd.check_format(BfdFormat::bfd_object).unwrap();
        bfd
    }

    #[test]
    fn test_symbol_count() {
        let mut bfd = open_test_object();
        // BFD minisymbols skip the initial null entry (readelf shows 19, BFD returns 18)
        let count = bfd.get_symbol_count();
        assert_eq!(count, 18);
    }

    #[test]
    fn test_symbol_names() {
        let mut bfd = open_test_object();

        // Index 0: file symbol
        let sym = bfd.get_symbol_at(0).unwrap();
        assert_eq!(sym.name(), "thunar-notify.c");

        // Index 1: .text section symbol
        let sym = bfd.get_symbol_at(1).unwrap();
        assert_eq!(sym.name(), ".text");

        // Index 3: local function
        let sym = bfd.get_symbol_at(3).unwrap();
        assert_eq!(sym.name(), "_sub_I_00099_0");
    }

    #[test]
    fn test_global_functions() {
        let mut bfd = open_test_object();

        // Index 9..=15 are global functions (type 'T')
        let expected = [
            "thunar_notify_progress",
            "thunar_notify_unmount",
            "thunar_notify_eject",
            "thunar_notify_finish",
            "thunar_notify_undo",
            "thunar_notify_redo",
            "thunar_notify_uninit",
        ];

        for (i, &name) in expected.iter().enumerate() {
            let sym = bfd.get_symbol_at((9 + i) as u32).unwrap();
            assert_eq!(sym.name(), name);
            assert_eq!(sym.type_char(), 'T');
            assert!(!sym.is_undefined());
        }
    }

    #[test]
    fn test_symbol_values() {
        let mut bfd = open_test_object();

        // thunar_notify_eject at offset 0x20
        let sym = bfd.get_symbol_at(11).unwrap();
        assert_eq!(sym.name(), "thunar_notify_eject");
        assert_eq!(sym.value(), 0x20);
    }

    #[test]
    fn test_undefined_symbols() {
        let mut bfd = open_test_object();

        // Index 16: __asan_init (undefined, type 'U')
        let sym = bfd.get_symbol_at(16).unwrap();
        assert_eq!(sym.name(), "__asan_init");
        assert!(sym.is_undefined());
        assert_eq!(sym.type_char(), 'U');
        assert_eq!(sym.value(), 0);
        assert_eq!(sym.sec_name(), "*UND*");
    }

    #[test]
    fn test_section_symbols() {
        let mut bfd = open_test_object();

        // Index 4..=8: debug section symbols
        let debug_sections = [
            ".debug_info",
            ".debug_abbrev",
            ".debug_line",
            ".debug_str",
            ".debug_line_str",
        ];
        for (i, &sec) in debug_sections.iter().enumerate() {
            let sym = bfd.get_symbol_at((4 + i) as u32).unwrap();
            assert_eq!(sym.name(), sec);
            assert_eq!(sym.sec_name(), sec);
            assert_eq!(sym.type_char(), 'N');
        }
    }

    #[test]
    fn test_file_symbol() {
        let mut bfd = open_test_object();

        // Index 0 is the FILE symbol (type 'a' = absolute)
        let sym = bfd.get_symbol_at(0).unwrap();
        assert_eq!(sym.name(), "thunar-notify.c");
        assert_eq!(sym.type_char(), 'a');
        assert!(!sym.is_undefined());
        assert_eq!(sym.sec_name(), "*ABS*");
    }

    #[test]
    fn test_out_of_bounds() {
        let mut bfd = open_test_object();

        assert!(bfd.get_symbol_at(100).is_none());
    }

    #[test]
    fn test_flags() {
        let mut bfd = open_test_object();

        // Global function should have non-zero flags
        let sym = bfd.get_symbol_at(9).unwrap();
        assert!(!sym.is_undefined());
        assert!(sym.flags() > 0);
    }
}

