<?php
namespace App\Http\Controllers;
use App\Models\Device;
use App\Models\Provider;
use App\Models\Region;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;
class AdminController extends Controller {
    private function guard(): void { abort_unless(auth()->user()?->isAdmin(),403); }
    public function index(){
        $this->guard();return view('admin.index', ['providers'=>Provider::with('regions')->orderBy('name')->get(), 'regions'=>Region::all(),'devices'=>Device::with('provider')->orderByDesc('id')->get()]);
    }
    public function storeProvider(Request $request){
        $this->guard();$data=$request->validate([
            'name'=>'required|string|max:180','city'=>'required|string|max:120','postal_code'=>'nullable|string|max:10',
            'phone'=>'nullable|string|max:50','regions'=>'required|array|min:1',
            'regions.*'=>'required|integer|exists:regions,id',
        ]);
        DB::transaction(function()use($data){
            $p=Provider::create(['name'=>$data['name'],'city'=>$data['city'],'postal_code'=>$data['postal_code']??null,'phone'=>$data['phone']??null,'status'=>'unavailable','capacity'=>0]);
            $p->regions()->sync($data['regions']);
        });
        return back()->with('notice','Dienstleister angelegt.');
    }
    public function storeDevice(Request $request){
        $this->guard();$data=$request->validate(['provider_id'=>'required|exists:providers,id','device_uid'=>'required|string|max:80|alpha_dash|unique:devices,device_uid']);
        $token=Str::random(64);
        Device::create(['provider_id'=>$data['provider_id'],'device_uid'=>$data['device_uid'],'token_hash'=>hash('sha256',$token)]);
        // Token deliberately shown once, not stored in plaintext or flashed into a session.
        return view('admin.token', ['token'=>$token,'uid'=>$data['device_uid']]);
    }
}
