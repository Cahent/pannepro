<?php
namespace App\Http\Controllers;
use App\Models\User;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Auth;
use Illuminate\Validation\Rules\Password;
class AuthController extends Controller {
    public function showLogin(){return view('auth.login');}
    public function showRegister(){return view('auth.register');}
    public function register(Request $request){
        $data=$request->validate(['name'=>'required|string|max:120','email'=>'required|email|max:255|unique:users,email','password'=>['required','confirmed',Password::min(12)]]);
        $user=User::create([...$data,'role'=>'customer']);
        Auth::login($user); $request->session()->regenerate();
        return redirect()->route('dashboard');
    }
    public function login(Request $request){
        $data=$request->validate(['email'=>'required|email','password'=>'required|string']);
        if (!Auth::attempt($data)) return back()->withErrors(['email'=>'Anmeldedaten sind ungültig.'])->onlyInput('email');
        $request->session()->regenerate();return redirect()->intended(route('dashboard'));
    }
    public function logout(Request $request){Auth::logout();$request->session()->invalidate();$request->session()->regenerateToken();return redirect()->route('login');}
}
