# Bulk migration helper for quickly renaming file extensions and replacing text.
import os
import argparse
from pathlib import Path

def replace_file_extension(directory, from_ext, to_ext):
    """
    Rename files with a specific extension to a new extension.
    
    Args:
        directory: Root directory to search
        from_ext: Original file extension (without dot)
        to_ext: New file extension (without dot)
    """
    # Ensure extensions don't have leading dots
    from_ext = from_ext.lstrip('.')
    to_ext = to_ext.lstrip('.')
    
    count = 0
    for root, dirs, files in os.walk(directory):
        for filename in files:
            if filename.endswith(f'.{from_ext}'):
                old_path = os.path.join(root, filename)
                new_filename = filename[:-len(from_ext)] + to_ext
                new_path = os.path.join(root, new_filename)
                
                try:
                    os.rename(old_path, new_path)
                    print(f"Renamed: {old_path} -> {new_path}")
                    count += 1
                except Exception as e:
                    print(f"Error renaming {old_path}: {e}")
    
    print(f"\nTotal files renamed: {count}")
    return count

def replace_in_files(directory, replace_list, with_list, file_extensions=None):
    """
    Replace text content in files.
    
    Args:
        directory: Root directory to search
        replace_list: List of strings to replace
        with_list: List of replacement strings
        file_extensions: Optional list of file extensions to process (e.g., ['txt', 'py'])
    """
    if len(replace_list) != len(with_list):
        print("Error: Number of replace and with values must match")
        return 0
    
    count = 0
    files_modified = 0
    
    for root, dirs, files in os.walk(directory):
        for filename in files:
            # Skip if file extension filter is set and file doesn't match
            if file_extensions:
                if not any(filename.endswith(f'.{ext}') for ext in file_extensions):
                    continue
            
            file_path = os.path.join(root, filename)
            
            try:
                # Try to read as text file
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                original_content = content
                
                # Perform all replacements
                for old_text, new_text in zip(replace_list, with_list):
                    content = content.replace(old_text, new_text)
                
                # Write back if content changed
                if content != original_content:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    
                    replacements_made = sum(original_content.count(old) for old in replace_list)
                    print(f"Modified: {file_path} ({replacements_made} replacements)")
                    files_modified += 1
                    count += replacements_made
                    
            except Exception as e:
                # Skip binary files or files that can't be read
                pass
    
    print(f"\nTotal replacements: {count} in {files_modified} files")
    return count

def main():
    parser = argparse.ArgumentParser(
        description='Replace file extensions and/or text content in files recursively'
    )
    parser.add_argument('directory', nargs='?', default='.', 
                        help='Directory to process (default: current directory)')
    parser.add_argument('--from', dest='from_ext', 
                        help='Original file extension to rename')
    parser.add_argument('--to', dest='to_ext', 
                        help='New file extension')
    parser.add_argument('--replace', 
                        help='Comma-separated list of strings to replace')
    parser.add_argument('--with', dest='with_text', 
                        help='Comma-separated list of replacement strings')
    parser.add_argument('--extensions', 
                        help='Comma-separated list of file extensions to process for content replacement (e.g., txt,py,md)')
    parser.add_argument('--dry-run', action='store_true',
                        help='Preview changes without making them')
    parser.add_argument('--helper', action='store_true',
                        help='Show extended usage information and exit')
    
    args = parser.parse_args()

    if args.helper:
        print("""Quarisma Bulk File Utility
-------------------------
Use this script to automate two workflows:
  1. Rename file extensions: --from .old --to .new
  2. Replace text in files: --replace foo,bar --with baz,qux [--extensions cpp,h]

Examples:
  python main.py Library --from .cpp --to .cpp
  python main.py . --replace c10::,at:: --with quarisma::,quarisma:: --extensions h,cpp

Combine --dry-run with either mode to preview changes without touching disk.""")
        return
    
    directory = args.directory
    
    if not os.path.isdir(directory):
        print(f"Error: {directory} is not a valid directory")
        return
    
    print(f"Processing directory: {os.path.abspath(directory)}\n")
    
    # Handle file extension replacement
    if args.from_ext and args.to_ext:
        if args.dry_run:
            print("DRY RUN: Would rename files with extension changes")
        else:
            replace_file_extension(directory, args.from_ext, args.to_ext)
    elif args.from_ext or args.to_ext:
        print("Error: Both --from and --to must be specified for extension replacement")
        return
    
    # Handle content replacement
    if args.replace and args.with_text:
        replace_list = [s.strip() for s in args.replace.split(',')]
        with_list = [s.strip() for s in args.with_text.split(',')]
        
        extensions = None
        if args.extensions:
            extensions = [ext.strip().lstrip('.') for ext in args.extensions.split(',')]
        
        if args.dry_run:
            print("\nDRY RUN: Would replace content in files")
        else:
            print()
            replace_in_files(directory, replace_list, with_list, extensions)
    elif args.replace or args.with_text:
        print("Error: Both --replace and --with must be specified for content replacement")
        return
    
    if not any([args.from_ext, args.to_ext, args.replace, args.with_text]):
        parser.print_help()

if __name__ == "__main__":
    main()
